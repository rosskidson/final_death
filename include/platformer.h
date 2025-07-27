#pragma once

#include "game_configuration.h"
#include "olcPixelGameEngine.h"

namespace platformer {

class Platformer : public olc::PixelGameEngine {
 public:
  Platformer();
  bool OnUserCreate() override;
  bool OnUserUpdate(float fElapsedTime) override;

 private:
  void Render(const double x, const double y);
  GameConfiguration config_;
};

}  // namespace platformer