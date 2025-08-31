#pragma once

#include <map>
#include <string>

#include "basic_types.h"
#include "game_configuration.h"
#include "olcPixelGameEngine.h"
#include "physics_engine.h"
#include "rendering_engine.h"

namespace platformer {

class Platformer : public olc::PixelGameEngine {
 public:
  Platformer();
  bool OnUserCreate() override;
  bool OnUserUpdate(float fElapsedTime) override;

 private:
  void Keyboard();
  Level& GetCurrentLevel() { return config_.levels.at(level_idx_); };

  GameConfiguration config_;
  int level_idx_;
  std::unique_ptr<RenderingEngine> rendering_engine_;
  std::unique_ptr<PhysicsEngine> physics_engine_;

  Player player_{};

  // TODO:: You will need to rethink this one.
  std::map<std::string, olc::Sprite> sprite_storage_;
};

}  // namespace platformer