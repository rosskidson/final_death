#pragma once

#include <map>
#include <string>

#include "basic_types.h"
#include "game_configuration.h"
#include "input_processor.h"
#include "olcPixelGameEngine.h"
#include "physics_engine.h"
#include "registry.h"
#include "registry_helpers.h"
#include "rendering_engine.h"
#include "sound.h"
#include "utils/parameter_server.h"
#include "utils/rate_timer.h"
#include "utils/simple_profiler.h"
#include "utils/windows_high_res_timer.h"

namespace platformer {

class Platformer : public olc::PixelGameEngine {
 public:
  Platformer();
  bool OnUserCreate() override;
  bool OnUserUpdate(float fElapsedTime) override;
  bool OnUserDestroy() override;
  bool OnConsoleCommand(const std::string& sCommand) override;

 private:
  bool Keyboard();
  Level& GetCurrentLevel() { return config_.levels.at(level_idx_); };

  GameConfiguration config_;
  int level_idx_;
  // TODO:: Remove all unique ptrs. They are like this for delayed initialization
  // (constructor requires resources not available at Platformer constructor time)
  // Change to a static creation factory pattern.
  std::shared_ptr<Registry> registry_;
  std::unique_ptr<RenderingEngine> rendering_engine_;
  std::unique_ptr<PhysicsEngine> physics_engine_;
  std::shared_ptr<ParameterServer> parameter_server_;
  std::unique_ptr<InputProcessor> input_processor_;
  std::shared_ptr<SoundPlayer> sound_player_;

  RateTimer rate_;
  SimpleProfiler profiler_;

  EntityId player_id_;

  #ifdef _WIN32
  WindowsHighResTimer high_res_timer_{1};
  #endif
};

}  // namespace platformer