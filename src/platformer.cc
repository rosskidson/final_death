#include "platformer.h"

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
constexpr double kJumpVel = 17.0;
constexpr double kFollowPlayerScreenRatio = 0.3;

namespace platformer {

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

  return parameter_server;
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

void LoadSprite(const std::string& filename,
                const std::string& name,
                std::map<std::string, olc::Sprite>& sprite_storage) {
  const auto sprite_path = std::filesystem::path(SOURCE_DIR) / "assets" / filename;
  sprite_storage[name] = olc::Sprite();
  if (sprite_storage[name].LoadFromFile(sprite_path.string()) != olc::rcode::OK) {
    std::cout << " Failed loading sprite `" << sprite_path << "`." << std::endl;
  }
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
  physics_engine_ = std::make_unique<PhysicsEngine>(GetCurrentLevel(), parameter_server_);

  player_.position = {10, 10};
  player_.velocity = {0, 0};

  std::vector<ActionSpriteSheet> aa;
  const auto player_path = std::filesystem::path(SOURCE_DIR) / "assets" / "player";
  // TODO:: emplace back never fucking works // clean up this shit somehow
  // Probably best to do the AnimatedSprite initialization in the constructor of animation manager
  // how I originally planned.
  aa.push_back(ActionSpriteSheet{
      Action::Idle, AnimatedSprite{(player_path / "zomdude-idle.png").string(), 48, true, 200}});
  aa.push_back(ActionSpriteSheet{
      Action::Walk, AnimatedSprite{(player_path / "zomdude-walk.png").string(), 48, true, 200}});
  aa.push_back(ActionSpriteSheet{
      Action::Shoot,
      AnimatedSprite{(player_path / "zomdude-fire-shotgun.png").string(), 48, false, 100}});

  player_.animation_manager = AnimationManager(std::move(aa));

  // TODO:: Configure this a bit better.
  // zomdude params
  player_.x_offset_px = 10;
  player_.y_offset_px = 0;
  player_.collision_width_px = 19;
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

  // TODO:: Fixed frame rate.
  std::this_thread::sleep_for(std::chrono::milliseconds(8));
  return true;
}

bool Platformer::OnConsoleCommand(const std::string& sCommand) {
  DeveloperConsole(sCommand, parameter_server_);
  return true;
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

  if (this->GetKey(olc::Key::LEFT).bHeld) {
    player_.acceleration.x = -acceleration;
  } else if (this->GetKey(olc::Key::RIGHT).bHeld) {
    player_.acceleration.x = +acceleration;
  } else {
    player_.animation_manager.EndAction(Action::Walk);
    DecelerateAndStopPlayer(player_, deceleration);
  }

  // if (this->GetKey(olc::Key::UP).bHeld) {
  //   player_.acceleration.y = acceleration;
  // } else if (this->GetKey(olc::Key::DOWN).bHeld) {
  //   player_.acceleration.y = acceleration;
  // } else {
  //   player_.acceleration.y = 0;
  // }

  if (this->GetKey(olc::Key::SPACE).bPressed) {
    player_.velocity.y = jump_velocity;
  }
  if (this->GetKey(olc::Key::CTRL).bPressed) {
    player_.animation_manager.StartAction(Action::Shoot);
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
