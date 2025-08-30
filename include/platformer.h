#pragma once

#include <map>
#include <string>

#include "basic_types.h"
#include "camera.h"
#include "game_configuration.h"
#include "olcPixelGameEngine.h"

namespace platformer {

enum class Side { LEFT, RIGHT, TOP, BOTTOM };

class Platformer : public olc::PixelGameEngine {
 public:
  Platformer();
  bool OnUserCreate() override;
  bool OnUserUpdate(float fElapsedTime) override;

 private:
  void CollisionCheckPlayer(Player& player);
  bool PlayerCollidesWithMap(Player& player);
  void Keyboard();
  Level& GetCurrentLevel() { return config_.levels.at(level_idx_); };

  GameConfiguration config_;
  int level_idx_;
  std::unique_ptr<Camera> camera_;

  Player player_{};

  // TODO:: You will need to rethink this one.
  std::map<std::string, olc::Sprite> sprite_storage_;
};

}  // namespace platformer