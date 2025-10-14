#include "platformer.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <thread>

#include "animated_sprite.h"
#include "animation_manager.h"
#include "basic_types.h"
#include "components.h"
#include "config.h"
#include "game_configuration.h"
#include "global_defs.h"
#include "input_processor.h"
#include "load_game_configuration.h"
#include "player_state.h"
#include "registry.h"
#include "registry_helpers.h"
#include "rendering_engine.h"
#include "sound.h"
#include "update_player_state.h"
#include "utils/developer_console.h"
#include "utils/game_clock.h"
#include "utils/logging.h"
#include "utils/parameter_server.h"

// TODO:: Int parameters
constexpr double kShootDelayMs = 1000;
constexpr double kRollDurationMs = 250;
constexpr double kShootDownUpwardVel = 10;
constexpr double kHardFallDistance = 10;

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
  PlayerState action;
};

EntityId InitializePlayer(Registry& registry) {
  auto id = registry.AddComponents(Position{2, 10},                                       //
                                   Velocity{0, 0},                                        //
                                   Acceleration{0, 0},                                    //
                                   FacingDirection{Direction::RIGHT},                     //
                                   CollisionBox{30, 0, 18, 48},                           //
                                   Collision{},                                           //
                                   State{PlayerState::Idle, GameClock::NowGlobal(), {}},  //
                                   DistanceFallen{0},                                     //
                                   PlayerComponent{});                                    //
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

  return parameter_server;
}

bool InitializePlayerAnimationManager(const ParameterServer& parameter_server,
                                      EntityId player_id,
                                      Registry& registry) {
  auto [player] = registry.GetComponents<PlayerComponent>(player_id);
  const auto player_path = std::filesystem::path(SOURCE_DIR) / "assets" / "player";
  constexpr int kWidth = 80;
  std::vector<AnimationInfo> animations = {
      // path, loops, start_frame_idx, end_frame_idx, forwards/backwards, player state
      {player_path / "player_idle_standing.png", true, 0, -1, -1, false, PlayerState::Idle},
      {player_path / "player_walk.png", true, 0, -1, -1, false, PlayerState::Walk},
      {player_path / "player_fire_standing.png", false, 1, -1, -1, false, PlayerState::Shoot},
      {player_path / "player_fire_jumping.png", false, 0, -1, -1, false, PlayerState::InAirShot},
      {player_path / "player_fire_crouch.png", false, 0, -1, -1, false, PlayerState::CrouchShot},
      {player_path / "player_fire_jumping_downshot.png", false, 0, -1, -1, false,
       PlayerState::InAirDownShot},
      {player_path / "player_idle_crouch.png", true, 0, -1, -1, false, PlayerState::Crouch},
      // TODO:: The first frame of aim up has been deleted as there is special logic missing to only
      // play it the on the transition from idle to up.
      {player_path / "player_idle_up.png", true, 0, -1, 0, false, PlayerState::AimUp},
      {player_path / "player_fire_upwards.png", false, 2, -1, -1, false, PlayerState::UpShot},
      {player_path / "player_roll.png", false, 1, 6, -1, false, PlayerState::PreRoll},
      {player_path / "player_roll.png", true, 7, 10, -1, false, PlayerState::Roll},
      {player_path / "player_roll.png", false, 11, 15, -1, false, PlayerState::PostRoll},
      {player_path / "player_jump.png", false, 1, 1, -1, false, PlayerState::PreJump},
      {player_path / "player_jump_dust_h.png", false, 0, -1, -1, false, PlayerState::HardLanding},
      {player_path / "player_jump_dust_l.png", false, 0, -1, -1, false, PlayerState::SoftLanding},
      {player_path / "player_jump.png", true, 2, 4, -1, true, PlayerState::InAir},
      {player_path / "player_fire_killself_count.png", false, 0, -1, -1, false,
       PlayerState::PreSuicide},
      {player_path / "player_fire_killself_fire.png", false, 0, -1, -1, false,
       PlayerState::Suicide},
      {player_path / "player_fire_backshot.png", false, 0, -1, -1, false, PlayerState::BackShot},
      {player_path / "player_fire_backdodge.png", false, 0, -1, -1, false,
       PlayerState::BackDodgeShot},
  };

  for (const auto& animation : animations) {
    auto animated_sprite = AnimatedSprite::CreateAnimatedSprite(
        animation.sprite_path, animation.loops, animation.start_frame_idx, animation.end_frame_idx,
        animation.intro_frames, animation.forwards_backwards);
    if (!animated_sprite.has_value()) {
      return false;
    }
    player.animation_manager.AddAnimation(std::move(*animated_sprite), animation.action);
  }

  return true;
}

void SetAnimationCallbacks(EntityId id,
                           std::shared_ptr<SoundPlayer> sound_player,
                           Registry& registry) {
  auto [player] = registry.GetComponents<PlayerComponent>(id);

  player.animation_manager.GetAnimation(PlayerState::Shoot).AddCallback(0, [=]() {
    sound_player->PlaySample("shotgun_fire", false);
  });
  player.animation_manager.GetAnimation(PlayerState::Shoot).AddCallback(5, [=]() {
    sound_player->PlaySample("shotgun_reload", false);
  });
  player.animation_manager.GetAnimation(PlayerState::InAirShot).AddCallback(0, [=]() {
    sound_player->PlaySample("shotgun_fire", false);
  });
  player.animation_manager.GetAnimation(PlayerState::InAirShot).AddCallback(5, [=]() {
    sound_player->PlaySample("shotgun_reload", false);
  });
  player.animation_manager.GetAnimation(PlayerState::CrouchShot).AddCallback(0, [=]() {
    sound_player->PlaySample("shotgun_fire", false);
  });
  player.animation_manager.GetAnimation(PlayerState::CrouchShot).AddCallback(5, [=]() {
    sound_player->PlaySample("shotgun_reload", false);
  });
  player.animation_manager.GetAnimation(PlayerState::PreSuicide).AddCallback(0, [=]() {
    sound_player->PlaySample("shotgun_reload", false);
  });
  player.animation_manager.GetAnimation(PlayerState::Suicide).AddCallback(0, [=]() {
    sound_player->PlaySample("shotgun_fire", false);
  });
  player.animation_manager.GetAnimation(PlayerState::UpShot).AddCallback(0, [=]() {
    sound_player->PlaySample("shotgun_fire", false);
  });
  player.animation_manager.GetAnimation(PlayerState::UpShot).AddCallback(5, [=]() {
    sound_player->PlaySample("shotgun_reload", false);
  });
  player.animation_manager.GetAnimation(PlayerState::BackShot).AddCallback(1, [=]() {
    sound_player->PlaySample("shotgun_fire", false);
  });
  player.animation_manager.GetAnimation(PlayerState::BackShot).AddCallback(6, [=]() {
    sound_player->PlaySample("shotgun_reload", false);
  });
  player.animation_manager.GetAnimation(PlayerState::BackDodgeShot).AddCallback(6, [=]() {
    sound_player->PlaySample("shotgun_fire", false);
  });
  player.animation_manager.GetAnimation(PlayerState::BackDodgeShot).AddCallback(9, [=]() {
    sound_player->PlaySample("shotgun_reload", false);
  });
  // player.animation_manager.GetAnimation(PlayerState::BackDodgeShot).AddCallback(0, [&]() {
  //   player.velocity.x = (player_.facing == Direction::LEFT) ? 100 : -100;
  // });

  // const auto shoot_down_upward_vel =
  //     parameter_server_->GetParameter<double>("physics/shoot.down.upward.vel");
  // player_.animation_manager.GetAnimation(PlayerState::InAirDownShot)
  //     .AddCallback(0, [&, shoot_down_upward_vel]() {
  //       sound_player_->PlaySample("shotgun_fire", false);
  //       player_.velocity.y += shoot_down_upward_vel;
  //     });
  // player_.animation_manager.GetAnimation(PlayerState::InAirDownShot).AddCallback(5, [&]() {
  //   sound_player_->PlaySample("shotgun_reload", false);
  // });

  // const auto jump_velocity = parameter_server_->GetParameter<double>("physics/jump.velocity");
  // player_.animation_manager.GetAnimation(PlayerState::PreJump)
  //     .AddExpireCallback([&, jump_velocity]() {
  //       if (player_.collisions.bottom) {
  //         player_.velocity.y = jump_velocity;
  //       }
  //     });

  // player_.animation_manager.GetAnimation(PlayerState::PreJump).AddExpireCallback([&]() {
  //   player_.velocity.x = player_.cached_velocity.x;
  //   player_.cached_velocity.x = 0;
  // });
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

  // TODO(FOR RELEASE): Path is assumed to be cmake source. Store assets in the binary.
  const auto levels_path = std::filesystem::path(SOURCE_DIR) / "levels.json";
  auto config = platformer::LoadGameConfiguration(levels_path.string());
  RETURN_FALSE_IF_FAILED(config);
  config_ = std::move(*config);
  level_idx_ = 0;
  const auto& tile_grid = GetCurrentLevel().tile_grid;

  registry_ = std::make_shared<Registry>();

  LOG_SIMPLE("Loading backgrounds...");
  rendering_engine_ =
      std::make_unique<RenderingEngine>(this, GetCurrentLevel(), parameter_server_, registry_);
  const auto background_path = std::filesystem::path(SOURCE_DIR) / "assets" / "backgrounds";
  RETURN_FALSE_IF_FAILED(
      rendering_engine_->AddBackgroundLayer(background_path / "background.png", 4));

  LOG_SIMPLE("Loading sounds/music...");
  sound_player_ = CreateSoundPlayer();
  RETURN_FALSE_IF_FAILED(sound_player_);
  // sound_player_->PlaySample("music", true, 0.2);

  physics_engine_ =
      std::make_unique<PhysicsEngine>(GetCurrentLevel(), parameter_server_, registry_);
  input_processor_ =
      std::make_unique<InputProcessor>(parameter_server_, sound_player_, registry_, this);

  player_id_ = InitializePlayer(*registry_);

  RETURN_FALSE_IF_FAILED(
      InitializePlayerAnimationManager(*parameter_server_, player_id_, *registry_));
  SetAnimationCallbacks(player_id_, sound_player_, *registry_);

  LOG_SIMPLE("Initialization successful.");
  rate_.Reset();
  return true;
}

bool Platformer::OnUserUpdate(float fElapsedTime) {
  // TODO:: Actual dt, not just 1 / frequency
  const double delta_t = std::chrono::duration<double>(rate_.GetFrameDuration()).count();

  // profiler_.Reset();
  // Control
  RETURN_FALSE_IF_FAILED(input_processor_->ProcessInputs(player_id_));
  // profiler_.LogEvent("00_control");

  // Model
  UpdateState(*parameter_server_, player_id_, *registry_);
  UpdateComponentsFromState(*parameter_server_, *registry_);

  auto [player, state] = registry_->GetComponents<PlayerComponent, State>(player_id_);
  player.animation_manager.Update(state.state);

  physics_engine_->GravitySystem();
  physics_engine_->FrictionSystem(delta_t);
  physics_engine_->PhysicsSystem(delta_t);
  physics_engine_->SetFacingDirectionSystem();
  physics_engine_->SetDistanceFallen(delta_t);

  // profiler_.LogEvent("01_update_player_state");

  // View
  rendering_engine_->KeepPlayerInFrame(player_id_);
  rendering_engine_->RenderBackground();
  rendering_engine_->RenderTiles();
  rendering_engine_->RenderEntities();
  rendering_engine_->RenderForeground();
  // profiler_.LogEvent("02_render");

  // profiler_.PrintTimings();
  rate_.Sleep(false);
  return true;
}

bool Platformer::OnConsoleCommand(const std::string& sCommand) {
  DeveloperConsole(sCommand, parameter_server_);
  return true;
}

}  // namespace platformer
