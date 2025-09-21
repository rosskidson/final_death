#include "platformer.h"

#include <filesystem>
#include <memory>
#include <thread>

#include "animated_sprite.h"
#include "animation_manager.h"
#include "basic_types.h"
#include "config.h"
#include "game_configuration.h"
#include "global_defs.h"
#include "input_processor.h"
#include "load_game_configuration.h"
#include "rendering_engine.h"
#include "sound.h"
#include "utils/developer_console.h"
#include "utils/parameter_server.h"

constexpr int kPixelSize = 3;

constexpr double kAcceleration = 50.0;
constexpr double kJumpVel = 21.0;
constexpr double kFollowPlayerScreenRatio = 0.3;

// TODO:: Int parameters
constexpr double kShootDelayMs = 1000;

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
  bool forwards_backwards;
  Action action;
};

std::shared_ptr<ParameterServer> CreateParameterServer() {
  auto parameter_server = std::make_shared<ParameterServer>();
  parameter_server->AddParameter("physics/player.acceleration", kAcceleration,
                                 "Horizontal acceleration of the player, unit: tile/s²");
  parameter_server->AddParameter("physics/jump.velocity", kJumpVel,
                                 "The instantaneous vertical velocity when you jump, unit: tile/s");
  parameter_server->AddParameter(
      "rendering/follow.player.screen.ratio", kFollowPlayerScreenRatio,
      "How far the player can walk towards the side of the screen before the camera follows, as a "
      "percentage of the screen size. The larger the ratio, the more centered the player will be "
      "on the screen.");

  parameter_server->AddParameter("timing/shoot.delay", kShootDelayMs,
                                 "How long it takes to shoot and reload");

  return parameter_server;
}

bool InitializePlayerAnimationManager(const ParameterServer& parameter_server, Player& player) {
  const auto player_path = std::filesystem::path(SOURCE_DIR) / "assets" / "player";
  const int shoot_delay =
      static_cast<int>(parameter_server.GetParameter<double>("timing/shoot.delay"));

  constexpr int kWidth = 80;
  std::vector<AnimationInfo> animations = {
      // path, loops, start_frame_idx, end_frame_idx, forwards/backwards, action
      {player_path / "player_idle_standing.png", true, 0, -1, false, Action::Idle},
      {player_path / "player_walk.png", true, 0, -1, false, Action::Walk},
      {player_path / "player_fire_standing.png", false, 0, -1, false, Action::Shoot},
      {player_path / "player_idle_crouch.png", true, 0, -1, false, Action::Crouch},
      {player_path / "player_roll.png", false, 0, -1, false, Action::Roll},
      {player_path / "player_jump.png", false, 1, -1, false, Action::JumpCrouch},
      // {player_path / "player_jump.png", false, 2, 4, true, Action::Fly},
  };

  for (const auto& animation : animations) {
    auto animated_sprite = AnimatedSprite::CreateAnimatedSprite(
        animation.sprite_path, animation.loops, animation.start_frame_idx, animation.end_frame_idx,
        animation.forwards_backwards);
    if (!animated_sprite.has_value()) {
      return false;
    }
    player.animation_manager.AddAnimation(std::move(*animated_sprite), animation.action);
  }

  return true;
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

Platformer::Platformer() {
  this->Construct(kScreenWidthPx, kScreenHeightPx, kPixelSize, kPixelSize);
}

bool Platformer::OnUserDestroy() { return true; }

bool Platformer::OnUserCreate() {
  // TODO(FOR RELEASE): Path is assumed to be cmake source. Store it in the binary, or do a proper
  // install
  this->SetPixelMode(olc::Pixel::Mode::MASK);
  const auto levels_path = std::filesystem::path(SOURCE_DIR) / "levels.json";
  auto config = platformer::LoadGameConfiguration(levels_path.string());
  if (!config.has_value()) {
    std::cout << "Failed loading config " << std::endl;
    return false;
  }
  config_ = std::move(*config);
  level_idx_ = 0;
  const auto& tile_grid = GetCurrentLevel().tile_grid;

  rendering_engine_ = std::make_unique<RenderingEngine>(this, GetCurrentLevel());
  const auto background_path = std::filesystem::path(SOURCE_DIR) / "assets" / "backgrounds";
  RETURN_FALSE_IF_FAILED(
      rendering_engine_->AddBackgroundLayer(background_path / "background.png", 4));

  sound_player_ = CreateSoundPlayer();
  RETURN_FALSE_IF_FAILED(sound_player_);
  sound_player_->PlaySample("music", true, 0.3);

  parameter_server_ = CreateParameterServer();
  physics_engine_ = std::make_unique<PhysicsEngine>(GetCurrentLevel(), parameter_server_);
  input_processor_ = std::make_unique<InputProcessor>(parameter_server_, sound_player_, this);

  RETURN_FALSE_IF_FAILED(InitializePlayerAnimationManager(*parameter_server_, player_));

  player_.animation_manager.GetAnimation(Action::Shoot).AddCallback(1, [&]() {
    sound_player_->PlaySample("shotgun_fire", false);
  });
  player_.animation_manager.GetAnimation(Action::Shoot).AddCallback(5, [&]() {
    sound_player_->PlaySample("shotgun_reload", false);
  });

  const auto jump_velocity = parameter_server_->GetParameter<double>("physics/jump.velocity");
  player_.animation_manager.GetAnimation(Action::JumpCrouch).AddCallback(0, [&, jump_velocity]() {
    if (player_.collisions.bottom) {
      player_.velocity.y = jump_velocity;
    }
  });

  player_.position = {10, 10};
  player_.velocity = {0, 0};

  // TODO:: Configure this a bit better.
  player_.x_offset_px = 30;
  player_.y_offset_px = 0;
  player_.collision_width_px = 18;
  player_.collision_height_px = 48;

  return true;
}

bool Platformer::OnUserUpdate(float fElapsedTime) {
  const auto follow_ratio =
      parameter_server_->GetParameter<double>("rendering/follow.player.screen.ratio");

  // Control
  if (!input_processor_->ProcessInputs(player_)) {
    return false;
  }

  // Model
  player_.animation_manager.GetActiveAnimation().TriggerCallbacks();
  physics_engine_->PhysicsStep(player_);

  // std::cout << player_.velocity.x << std::endl;

  // View
  rendering_engine_->KeepPlayerInFrame(player_, follow_ratio);  // Split ratio to x/y
  rendering_engine_->RenderBackground();
  rendering_engine_->RenderTiles();
  rendering_engine_->RenderPlayer(player_);
  rendering_engine_->RenderForeground();

  // TODO:: Fix frame rate.
  std::this_thread::sleep_for(std::chrono::milliseconds(8));
  return true;
}

bool Platformer::OnConsoleCommand(const std::string& sCommand) {
  DeveloperConsole(sCommand, parameter_server_);
  return true;
}

}  // namespace platformer
