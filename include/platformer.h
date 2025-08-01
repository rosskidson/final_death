#pragma once

#include "basic_types.h"
#include "camera.h"
#include "game_configuration.h"
#include "olcPixelGameEngine.h"
#include "tileset.h"

namespace platformer {
class Platformer : public olc::PixelGameEngine {
 public:
  Platformer();
  bool OnUserCreate() override;
  bool OnUserUpdate(float fElapsedTime) override;

 private:
  void Render(const double x, const double y);
  void Keyboard();
  Level& GetCurrentLevel() { return config_.levels.at(level_); };

  GameConfiguration config_;
  int level_;
  std::unique_ptr<Camera> camera_;

  Player player_{};
};

}  // namespace platformer