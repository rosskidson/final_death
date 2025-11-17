#include "platformer.h"

#include <chrono>
#include <filesystem>
#include <memory>

#include "animation/animated_sprite.h"
#include "animation/simple_sprites.h"
#include "animation/sprite_manager.h"
#include "common_types/actor_state.h"
#include "common_types/components.h"
#include "common_types/entity.h"
#include "common_types/game_configuration.h"
#include "config.h"
#include "global_defs.h"
#include "input/input_processor.h"
#include "load_game_configuration.h"
#include "registry.h"
#include "sound/sound_player.h"
#include "sound/sound_processor.h"
#include "systems/player_logic_system.h"
#include "systems/rendering_system.h"
#include "utils/developer_console.h"
#include "utils/logging.h"
#include "utils/parameter_server.h"

// TODO(BT-01):: Int parameters
constexpr double kShootDelayMs = 1000;
constexpr double kRollDurationMs = 250;
constexpr double kShootDownUpwardVel = 10;
constexpr double kHardFallDistance = 10;
constexpr double kJumpVel = 21.0;

namespace platformer {

#define RETURN_NULL_PTR_ON_ERROR(statement) \
  if (!(statement)) {                       \
    return nullptr;                         \
  }

#define RETURN_FALSE_IF_FAILED(statement) \
  if (!(statement)) {                     \
    return false;                         \
  }

struct AnimationInfo {
  const std::filesystem::path sprite_path;
  bool loops;
  int start_frame_idx;
  int end_frame_idx;
  int intro_frames;
  bool forwards_backwards;
  State state;
};

std::string MakeKey(const Actor actor, const State state) {
  return ToString(actor) + "-" + ToString(state);
}

std::string MakePlayerKey(const State state) {
  return ToString(Actor::Player) + "-" + ToString(state);
}

EntityId InitializePlayer(Registry& registry) {
  auto id = registry.AddComponents(
      Position{2, 10},                               //
      Velocity{0, 0},                                //
      Acceleration{0, 0},                            //
      FacingDirection{Direction::RIGHT},             //
      CollisionBox{30, 0, 18, 48},                   //
      Collision{},                                   //
      StateComponent{Actor::Player, {State::Idle}},  //
      PlayerComponent{},                             //
      AnimatedSpriteComponent{
          GameClock::NowGlobal(), {}, MakeKey(Actor::Player, State::Idle)},  //// TODO REMOVE!!!!
      DistanceFallen{0});                                                    //
  return id;
}

std::shared_ptr<ParameterServer> CreateParameterServer() {
  auto parameter_server = std::make_shared<ParameterServer>();
  parameter_server->AddParameter("timing/shoot.delay", kShootDelayMs,
                                 "How long it takes to shoot and reload");
  parameter_server->AddParameter("timing/roll.duration.ms", kRollDurationMs,
                                 "How long the roll lasts.");

  parameter_server->AddParameter(
      "physics/shoot.down.upward.vel", kShootDownUpwardVel,
      "How much kickback the player should get when he fires his weapon down in the air.");

  parameter_server->AddParameter("physics/hard.fall.distance", kHardFallDistance,
                                 "Distance to trigger a hard fall (crouch + delay for recovery)");

  // TODO:: rename all .velocity with .vel
  parameter_server->AddParameter("physics/jump.velocity", kJumpVel,
                                 "The instantaneous vertical velocity when you jump, unit: tile/s");

  parameter_server->AddParameter("debug/enable.timing", 0., "Spam the console with timing debug");

  return parameter_server;
}

std::shared_ptr<SpriteManager> InitializeAnimationManager(const ParameterServer& parameter_server,
                                                          EntityId player_id,
                                                          std::shared_ptr<Registry> registry) {
  const auto player_path = std::filesystem::path(SOURCE_DIR) / "assets" / "player";
  constexpr int kWidth = 80;
  std::vector<AnimationInfo> animations = {
      // path, loops, start_frame_idx, end_frame_idx, forwards/backwards, player state
      {player_path / "player_idle_standing.png", true, 0, -1, -1, false, State::Idle},
      {player_path / "player_walk.png", true, 0, -1, -1, false, State::Walk},
      {player_path / "player_fire_standing.png", false, 1, -1, -1, false, State::Shoot},
      {player_path / "player_fire_jumping.png", false, 0, -1, -1, false, State::InAirShot},
      {player_path / "player_fire_crouch.png", false, 0, -1, -1, false, State::CrouchShot},
      {player_path / "player_fire_jumping_downshot.png", false, 0, -1, -1, false,
       State::InAirDownShot},
      {player_path / "player_idle_crouch.png", true, 0, -1, -1, false, State::Crouch},
      {player_path / "player_idle_up.png", true, 0, -1, 0, false, State::AimUp},
      {player_path / "player_fire_upwards.png", false, 2, -1, -1, false, State::UpShot},
      {player_path / "player_roll.png", false, 1, 6, -1, false, State::PreRoll},
      {player_path / "player_roll.png", true, 7, 10, -1, false, State::Roll},
      {player_path / "player_roll.png", false, 11, 15, -1, false, State::PostRoll},
      {player_path / "player_jump.png", false, 1, 1, -1, false, State::PreJump},
      {player_path / "player_jump_dust_h.png", false, 0, -1, -1, false, State::HardLanding},
      {player_path / "player_jump_dust_l.png", false, 0, -1, -1, false, State::SoftLanding},
      {player_path / "player_jump.png", true, 2, 4, -1, true, State::InAir},
      {player_path / "player_fire_killself_count.png", false, 0, -1, -1, false, State::PreSuicide},
      {player_path / "player_fire_killself_fire.png", false, 0, -1, -1, false, State::Suicide},
      {player_path / "player_fire_backshot.png", false, 0, -1, -1, false, State::BackShot},
      {player_path / "player_fire_backdodge.png", false, 0, -1, -1, false, State::BackDodgeShot},
  };

  auto animation_manager = std::make_shared<SpriteManager>(std::move(registry));
  for (const auto& animation : animations) {
    auto animated_sprite = AnimatedSprite::CreateAnimatedSprite(
        animation.sprite_path, animation.loops, animation.start_frame_idx, animation.end_frame_idx,
        animation.intro_frames, animation.forwards_backwards);
    if (!animated_sprite.has_value()) {
      return nullptr;
    }
    animation_manager->AddAnimation(MakeKey(Actor::Player, animation.state),
                                    std::move(*animated_sprite));
  }

  // Bullet
  {
    auto animated_sprite = AnimatedSprite::CreateAnimatedSprite(
        player_path / "misc_animated_bullet_01.png", true, 0, -1, -1, 8, 3);
    if (!animated_sprite.has_value()) {
      return nullptr;
    }
    animation_manager->AddAnimation("bullet_01", std::move(*animated_sprite));
  }

  // Bullet vertical
  {
    auto animated_sprite = AnimatedSprite::CreateAnimatedSprite(
        player_path / "misc_animated_bullet_v_01.png", true, 0, -1, -1, 2, 7);
    if (!animated_sprite.has_value()) {
      return nullptr;
    }
    animation_manager->AddAnimation("bullet_v_01", std::move(*animated_sprite));
  }

  // shotgun pellet
  animation_manager->AddSprite("pellet", 1, 2, CreateShotgunPelletSprite());

  animation_manager->AddInsideSpriteLocation(MakePlayerKey(State::BackDodgeShot), {58, 12});
  animation_manager->AddInsideSpriteLocation(MakePlayerKey(State::BackShot), {9, 27});
  animation_manager->AddInsideSpriteLocation(MakePlayerKey(State::CrouchShot), {62, 19});
  animation_manager->AddInsideSpriteLocation(MakePlayerKey(State::InAirShot), {61, 27});
  animation_manager->AddInsideSpriteLocation(MakePlayerKey(State::InAirDownShot), {46, 11});
  animation_manager->AddInsideSpriteLocation(MakePlayerKey(State::Shoot), {62, 36});
  animation_manager->AddInsideSpriteLocation(MakePlayerKey(State::UpShot), {43, 47});

  return animation_manager;
}

void SetAnimationCallbacks(SpriteManager& animation_manager) {
  auto& a = animation_manager;
  const std::string shoot = "PlayerShoot";
  const std::string reload = "ReloadShotgun";
  a.GetAnimation(MakePlayerKey(State::Shoot)).AddEventSignal(0, shoot);
  a.GetAnimation(MakePlayerKey(State::Shoot)).AddEventSignal(5, reload);
  a.GetAnimation(MakePlayerKey(State::InAirShot)).AddEventSignal(0, shoot);
  a.GetAnimation(MakePlayerKey(State::InAirShot)).AddEventSignal(5, reload);
  a.GetAnimation(MakePlayerKey(State::InAirDownShot)).AddEventSignal(0, shoot);
  a.GetAnimation(MakePlayerKey(State::InAirDownShot)).AddEventSignal(5, reload);
  a.GetAnimation(MakePlayerKey(State::CrouchShot)).AddEventSignal(0, shoot);
  a.GetAnimation(MakePlayerKey(State::CrouchShot)).AddEventSignal(5, reload);
  a.GetAnimation(MakePlayerKey(State::UpShot)).AddEventSignal(0, shoot);
  a.GetAnimation(MakePlayerKey(State::UpShot)).AddEventSignal(5, reload);
  a.GetAnimation(MakePlayerKey(State::BackShot)).AddEventSignal(1, shoot);
  a.GetAnimation(MakePlayerKey(State::BackShot)).AddEventSignal(6, reload);
  a.GetAnimation(MakePlayerKey(State::BackDodgeShot)).AddEventSignal(6, shoot);
  a.GetAnimation(MakePlayerKey(State::BackDodgeShot)).AddEventSignal(9, reload);
  a.GetAnimation(MakePlayerKey(State::PreSuicide)).AddEventSignal(0, reload);
  a.GetAnimation(MakePlayerKey(State::Suicide)).AddEventSignal(0, shoot);
  a.GetAnimation(MakePlayerKey(State::InAirDownShot)).AddEventSignal(0, "ShootShotgunDownInAir");
  a.GetAnimation(MakePlayerKey(State::BackDodgeShot)).AddEventSignal(0, "StartBackDodgeShot");
}

std::shared_ptr<SoundPlayer> CreateSoundPlayer() {
  auto player = std::make_shared<SoundPlayer>();
  const auto path = std::filesystem::path(SOURCE_DIR) / "assets" / "sounds";
  RETURN_NULL_PTR_ON_ERROR(
      player->LoadWavFromFilesystem(path / "sfx_shotgun_shot.wav", "shotgun_fire"));
  RETURN_NULL_PTR_ON_ERROR(
      player->LoadWavFromFilesystem(path / "sfx_shotgun_reload.wav", "shotgun_reload"));

  const auto music_path = std::filesystem::path(SOURCE_DIR) / "assets" / "music";
  // Failing to load music is okay.
  (void)player->LoadWavFromFilesystem(music_path / "welcome_to_the_hub.mp3", "music");
  return player;
}

Platformer::Platformer() : parameter_server_{CreateParameterServer()}, rate_(kGameFrequency) {
  this->Construct(kScreenWidthPx, kScreenHeightPx, kPixelSize, kPixelSize);
}

bool Platformer::OnUserDestroy() { return true; }

bool Platformer::OnUserCreate() {
  this->SetPixelMode(olc::Pixel::Mode::MASK);

  rng_ = std::make_shared<RandomNumberGenerator>(RandomNumberGenerator::Mode::Hardware);

  // TODO(BT-02): Path is assumed to be cmake source. Store assets in the binary.
  const auto levels_path = std::filesystem::path(SOURCE_DIR) / "levels.json";
  auto config = platformer::LoadGameConfiguration(levels_path.string());
  RETURN_FALSE_IF_FAILED(config);
  config_ = std::move(*config);
  level_idx_ = 0;

  registry_ = std::make_shared<Registry>();
  player_id_ = InitializePlayer(*registry_);

  LOG_SIMPLE("Loading sprites...");
  animation_manager_ = InitializeAnimationManager(*parameter_server_, player_id_, registry_);
  RETURN_FALSE_IF_FAILED(animation_manager_);
  SetAnimationCallbacks(*animation_manager_);

  LOG_SIMPLE("Loading backgrounds...");
  rendering_system_ = std::make_unique<RenderingSystem>(this, GetCurrentLevel(), parameter_server_,
                                                        animation_manager_, registry_);
  const auto background_path = std::filesystem::path(SOURCE_DIR) / "assets" / "backgrounds";
  RETURN_FALSE_IF_FAILED(
      rendering_system_->AddBackgroundLayer(background_path / "background.png", 4));

  LOG_SIMPLE("Loading sounds/music...");
  sound_player_ = CreateSoundPlayer();
  RETURN_FALSE_IF_FAILED(sound_player_);
  sound_processor_ = std::make_shared<SoundProcessor>(sound_player_);
  // sound_player_->PlaySample("music", true, 0.2);

  physics_system_ =
      std::make_unique<PhysicsSystem>(GetCurrentLevel(), parameter_server_, registry_);
  input_processor_ = std::make_unique<InputProcessor>(parameter_server_, registry_, this);
  projectile_system_ =
      std::make_unique<ProjectileSystem>(parameter_server_, animation_manager_, rng_, registry_,
                                         GetCurrentLevel().level_tileset->GetTileSize());

  LOG_SIMPLE("Initialization successful.");
  rate_.Reset();
  return true;
}

// Move this elsewhere
void Platformer::RemoveComponentsWithTimeToLive() {
  for (EntityId id : registry_->GetView<TimeToDespawn>()) {
    const auto& time_to_die = registry_->GetComponent<TimeToDespawn>(id).time_to_despawn;
    if (GameClock::NowGlobal() > time_to_die) {
      registry_->RemoveComponent(id);
    }
  }
}

void Platformer::UpdateAnimatedSpriteComponentFromState() {
  for (const EntityId id : registry_->GetView<AnimatedSpriteComponent, StateComponent>()) {
    const auto& state = registry_->GetComponentConst<StateComponent>(id);
    auto& animated_sprite = registry_->GetComponent<AnimatedSpriteComponent>(id);
    const auto new_key = MakeKey(state.actor_type, state.state.GetState());
    if (new_key != animated_sprite.key ||
        animated_sprite.start_time != state.state.GetStateSetAt()) {
      animated_sprite.key = new_key;
      animated_sprite.last_animation_frame_idx.Reset();
      animated_sprite.start_time = state.state.GetStateSetAt();
    }
  }
}

bool Platformer::OnUserUpdate(float fElapsedTime) {
  const double delta_t = std::chrono::duration<double>(rate_.GetFrameDuration()).count();
  profiler_.Reset();

  // Control
  RETURN_FALSE_IF_FAILED(input_processor_->ProcessInputs(player_id_));
  profiler_.LogEvent("00_control");

  // Model
  const auto events = animation_manager_->GetAnimationEvents();
  sound_processor_->ProcessAnimationEvents(events);
  UpdatePlayerState(*parameter_server_, events, *physics_system_, *registry_);
  SetFacingDirection(*registry_);
  UpdateComponentsFromState(*parameter_server_, *registry_);
  UpdatePlayerComponentsFromState(*parameter_server_, events, *registry_);
  UpdateAnimatedSpriteComponentFromState();
  projectile_system_->SpawnProjectiles(events);
  RemoveComponentsWithTimeToLive();
  profiler_.LogEvent("01_update_states");

  physics_system_->ApplyGravity();
  physics_system_->ApplyFriction(delta_t);
  physics_system_->PhysicsStep(delta_t);
  physics_system_->SetDistanceFallen(delta_t);
  profiler_.LogEvent("02_physics");

  // View
  rendering_system_->KeepPlayerInFrame(player_id_);
  rendering_system_->RenderBackground();
  rendering_system_->RenderTiles();
  rendering_system_->RenderEntities();
  rendering_system_->RenderForeground();
  profiler_.LogEvent("03_render");

  if (parameter_server_->GetParameter<double>("debug/enable.timing") > 0) {
    profiler_.PrintTimings();
  }
  rate_.Sleep(false);
  return true;
}

bool Platformer::OnConsoleCommand(const std::string& sCommand) {
  DeveloperConsole(sCommand, parameter_server_);
  return true;
}

}  // namespace platformer
