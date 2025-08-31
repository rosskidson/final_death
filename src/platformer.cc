#include "platformer.h"

#include <chrono>
#include <map>
#include <memory>
#include <thread>

#include "basic_types.h"
#include "config.h"
#include "game_configuration.h"
#include "global_defs.h"
#include "load_game_configuration.h"
#include "rendering_engine.h"

using Clock = std::chrono::high_resolution_clock;

constexpr int kPixelSize = 3;

constexpr double kAcceleration = 50.0;
constexpr double kJumpVel = 17.0;
constexpr double kFollowPlayerScreenRatio = 0.3;

namespace platformer {

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
  parameter_server_ = std::make_shared<ParameterServer>();
  parameter_server_->AddParameter("physics/player_acceleration", kAcceleration,
                                  "Horizontal acceleration of the player, unit: tile/s²");
  parameter_server_->AddParameter(
      "physics/jump_velocity", kJumpVel,
      "The instantaneous vertical velocity when you jump, unit: tile/s");
  parameter_server_->AddParameter(
      "rendering/follow_player_screen_ratio", kFollowPlayerScreenRatio,
      "How far the player can walk towards the side of the screen before the camera follows, as a "
      "percentage of the screen size. The larger the ratio, the more centered the player will be "
      "on the screen.");

  rendering_engine_ = std::make_unique<RenderingEngine>(this, GetCurrentLevel());
  physics_engine_ = std::make_unique<PhysicsEngine>(GetCurrentLevel(), parameter_server_);

  player_.position = {10, 10};
  player_.velocity = {0, 0};

  LoadSprite("player/idle.png", "bot_idle", sprite_storage_);
  LoadSprite("player/jump.png", "bot_jump", sprite_storage_);
  LoadSprite("player/run1.png", "bot_run0", sprite_storage_);
  LoadSprite("player/run2.png", "bot_run1", sprite_storage_);
  LoadSprite("player/run3.png", "bot_run2", sprite_storage_);
  LoadSprite("player/run4.png", "bot_run3", sprite_storage_);
  player_.sprite = &sprite_storage_["bot_idle"];

  // TODO:: Configure this a bit better.
  player_.x_offset_px = 7;
  player_.y_offset_px = 0;
  player_.collision_width_px = 12;
  player_.collision_height_px = 22;

  return true;
}

bool Platformer::OnUserUpdate(float fElapsedTime) {
  this->Keyboard();
  physics_engine_->PhysicsStep(player_);

  const auto follow_ratio =
      parameter_server_->GetParameter<double>("rendering/follow_player_screen_ratio");
  rendering_engine_->KeepPlayerInFrame(player_, follow_ratio);
  rendering_engine_->RenderBackground();
  rendering_engine_->RenderTiles();
  rendering_engine_->RenderPlayer(player_);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  return true;
}

void Platformer::Keyboard() {
  Vector2d pos{};
  const auto tile_size = GetCurrentLevel().level_tileset->GetTileSize();
  const auto acceleration = parameter_server_->GetParameter<double>("physics/player_acceleration");
  const auto jump_velocity = parameter_server_->GetParameter<double>("physics/jump_velocity");

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

  auto now = Clock::now();
  if (this->GetKey(olc::Key::LEFT).bPressed) {
    player_.sprite = &sprite_storage_["bot_run1"];
    player_.animation_update = now;
  }
  if (this->GetKey(olc::Key::RIGHT).bPressed) {
    player_.sprite = &sprite_storage_["bot_run1"];
    player_.animation_update = now;
  }

  if (this->GetKey(olc::Key::LEFT).bHeld) {
    player_.acceleration.x = -acceleration;
    if (((now - player_.animation_update).count() / 1e9) > 0.1) {
      player_.animation_frame++;
      const std::string next_frame = "bot_run" + std::to_string(player_.animation_frame % 4);
      // std::cout << next_frame << std::endl;
      player_.sprite = &sprite_storage_[next_frame];
      player_.animation_update = now;
    }
  } else if (this->GetKey(olc::Key::RIGHT).bHeld) {
    player_.acceleration.x = +acceleration;
    if (((now - player_.animation_update).count() / 1e9) > 0.1) {
      player_.animation_frame++;
      const std::string next_frame = "bot_run" + std::to_string(player_.animation_frame % 4);
      // std::cout << next_frame << std::endl;
      player_.sprite = &sprite_storage_[next_frame];
      player_.animation_update = now;
    }
  } else {
    player_.sprite = &sprite_storage_["bot_idle"];
    player_.acceleration.x = -10 * player_.velocity.x;
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

  if (this->GetKey(olc::Key::Q).bReleased) {
    exit(0);
  }
}

}  // namespace platformer
