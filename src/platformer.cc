#include "platformer.h"

#include <filesystem>
#include <map>
#include <memory>
#include <thread>

#include "animated_sprite.h"
#include "animation_manager.h"
#include "basic_types.h"
#include "config.h"
#include "game_configuration.h"
#include "global_defs.h"
#include "load_game_configuration.h"
#include "rendering_engine.h"
#include "utils/developer_console.h"
#include "utils/game_clock.h"
#include "utils/parameter_server.h"

constexpr int kPixelSize = 3;

constexpr double kAcceleration = 50.0;
constexpr double kDeceleration = 10.0;
constexpr double kJumpVel = 21.0;
constexpr double kFollowPlayerScreenRatio = 0.3;

// TODO:: Int parameters
constexpr double kShootDelayMs = 500;

namespace platformer {

struct AnimationInfo {
  const std::filesystem::path sprite_path;
  int sprite_width;
  bool loops;
  bool forwards_backwards;
  int animation_duration_ms;
  Action action;
};

std::shared_ptr<ParameterServer> CreateParameterServer() {
  auto parameter_server = std::make_shared<ParameterServer>();
  parameter_server->AddParameter("physics/player.acceleration", kAcceleration,
                                 "Horizontal acceleration of the player, unit: tile/s²");
  parameter_server->AddParameter("physics/player.deceleration", kDeceleration,
                                 "Horizontal deceleration factor for the player. No unit.");
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
      // path, sprite width, loops, forwards/backwards, animation_duration_ms, action
      {player_path / "player_idle.png", kWidth, true, false, 1400, Action::Idle},
      {player_path / "player_walk.png", kWidth, true, true, 800, Action::Walk},
      {player_path / "player_fire_forwards.png", kWidth, false, false, shoot_delay, Action::Shoot},
      {player_path / "player_crouch.png", kWidth, false, false, 1400, Action::Crouch},
      {player_path / "player_roll.png", kWidth, false, false, 500, Action::Roll},
  };

  for (const auto& animation : animations) {
    auto animated_sprite = AnimatedSprite::CreateAnimatedSprite(
        animation.sprite_path, animation.sprite_width, animation.loops,
        animation.forwards_backwards, animation.animation_duration_ms);
    if (!animated_sprite.has_value()) {
      return false;
    }
    player.animation_manager.AddAnimation(std::move(*animated_sprite), animation.action);
  }
  return true;
}

void DecelerateAndStopPlayer(Player& player, const double deceleration) {
  if (std::abs(player.velocity.x) < 0.1) {
    player.acceleration.x = 0;
    player.velocity.x = 0;
    return;
  }
  player.acceleration.x = -player.velocity.x * deceleration;
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

  parameter_server_ = CreateParameterServer();

  rendering_engine_ = std::make_unique<RenderingEngine>(this, GetCurrentLevel());
  const auto background_path = std::filesystem::path(SOURCE_DIR) / "assets" / "backgrounds";
  if (!rendering_engine_->AddBackgroundLayer(background_path / "background.png", 4)) {
    return false;
  }
  // Forest parallax.
  // if (!rendering_engine_->AddBackgroundLayer(background_path / "forest" / "back.png", 4) ||
  //     !rendering_engine_->AddBackgroundLayer(background_path / "forest" / "middle.png", 2) ||
  //     !rendering_engine_->AddForegroundLayer(background_path / "forest" / "front.png", 0.5)) {
  //   return false;
  // }

  physics_engine_ = std::make_unique<PhysicsEngine>(GetCurrentLevel(), parameter_server_);

  if (!InitializePlayerAnimationManager(*parameter_server_, player_)) {
    return false;
  }
  player_.position = {10, 10};
  player_.velocity = {0, 0};

  // TODO:: Configure this a bit better.
  // zomdude params
  player_.x_offset_px = 30;
  player_.y_offset_px = 0;
  player_.collision_width_px = 18;
  player_.collision_height_px = 48;

  // Megaman params
  // player_.x_offset_px = 7;
  // player_.y_offset_px = 0;
  // player_.collision_width_px = 12;
  // player_.collision_height_px = 22;

  return true;
}

bool Platformer::OnUserUpdate(float fElapsedTime) {
  const auto follow_ratio =
      parameter_server_->GetParameter<double>("rendering/follow.player.screen.ratio");

  if (!this->Keyboard()) {
    return false;
  }

  if (!IsConsoleShowing()) {
    GameClock::ResumeGlobal();
  }

  physics_engine_->PhysicsStep(player_);

  rendering_engine_->KeepPlayerInFrame(player_, follow_ratio);
  rendering_engine_->RenderBackground();
  rendering_engine_->RenderTiles();
  rendering_engine_->RenderPlayer(player_);
  rendering_engine_->RenderForeground();

  // TODO:: Fixed frame rate.
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  return true;
}

bool Platformer::OnConsoleCommand(const std::string& sCommand) {
  DeveloperConsole(sCommand, parameter_server_);
  return true;
}

bool IsPlayerShooting(const Player& player, const ParameterServer& parameter_server) {
  const auto shoot_delay = parameter_server.GetParameter<double>("timing/shoot.delay");
  return (GameClock::NowGlobal() - player.started_shooting).count() / 1e6 < shoot_delay;
}

bool Platformer::Keyboard() {
  if (this->IsConsoleShowing()) {
    return true;
  }
  Vector2d pos{};
  const auto tile_size = GetCurrentLevel().level_tileset->GetTileSize();
  const auto acceleration = parameter_server_->GetParameter<double>("physics/player.acceleration");
  const auto deceleration = parameter_server_->GetParameter<double>("physics/player.deceleration");
  const auto jump_velocity = parameter_server_->GetParameter<double>("physics/jump.velocity");

  const double kCamMoveOffset = 1. / tile_size;
  if (this->GetKey(olc::Key::A).bHeld) {
    pos.x -= kCamMoveOffset;
  }
  if (this->GetKey(olc::Key::D).bHeld) {
    pos.x += kCamMoveOffset;
  }
  if (this->GetKey(olc::Key::W).bHeld) {
    pos.y += kCamMoveOffset;
  }
  if (this->GetKey(olc::Key::S).bHeld) {
    pos.y -= kCamMoveOffset;
  }
  rendering_engine_->MoveCamera(pos);

  auto now = GameClock::NowGlobal();
  if (this->GetKey(olc::Key::LEFT).bPressed) {
    player_.animation_manager.StartAction(Action::Walk);
  }
  if (this->GetKey(olc::Key::RIGHT).bPressed) {
    player_.animation_manager.StartAction(Action::Walk);
  }

  if (this->GetKey(olc::Key::LEFT).bHeld && !IsPlayerShooting(player_, *parameter_server_)) {
    player_.acceleration.x = -acceleration;
  } else if (this->GetKey(olc::Key::RIGHT).bHeld &&
             !IsPlayerShooting(player_, *parameter_server_)) {
    player_.acceleration.x = +acceleration;
  } else {
    if (!IsPlayerShooting(player_, *parameter_server_)) {
      player_.animation_manager.EndAction(Action::Walk);
    }
    DecelerateAndStopPlayer(player_, deceleration);
  }

  if (this->GetKey(olc::Key::DOWN).bPressed) {
    player_.animation_manager.StartAction(Action::Crouch);
  }
  if (this->GetKey(olc::Key::DOWN).bReleased) {
    player_.animation_manager.EndAction(Action::Crouch);
  }

  if (this->GetKey(olc::Key::SHIFT).bPressed) {
    player_.animation_manager.StartAction(Action::Roll);
  }

  // if (this->GetKey(olc::Key::UP).bHeld) {
  //   player_.acceleration.y = acceleration;
  // } else if (this->GetKey(olc::Key::DOWN).bHeld) {
  //   player_.acceleration.y = acceleration;
  // } else {
  //   player_.acceleration.y = 0;
  // }

  if (this->GetKey(olc::Key::SPACE).bPressed) {
    if (player_.collisions.bottom) {
      player_.velocity.y = jump_velocity;
    }
  }
  if (this->GetKey(olc::Key::CTRL).bPressed) {
    if (!IsPlayerShooting(player_, *parameter_server_)) {
      player_.velocity.x = 0;
      player_.started_shooting = GameClock::NowGlobal();
      player_.animation_manager.StartAction(Action::Shoot);
    }
  }

  if (this->GetKey(olc::Key::Q).bReleased) {
    return false;
  }

  if (this->GetKey(olc::Key::TAB).bPressed) {
    GameClock::PauseGlobal();
    this->ConsoleShow(olc::Key::TAB, false);
    this->ConsoleCaptureStdOut(true);
    PrintConsoleWelcome();
  }

  return true;
}

}  // namespace platformer
