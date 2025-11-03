#pragma once

#include <string>

#include "animation/animation_manager.h"
#include "common_types/game_configuration.h"
#include "input/input_processor.h"
#include "olcPixelGameEngine.h"
#include "registry.h"
#include "sound/sound_player.h"
#include "sound/sound_processor.h"
#include "systems/physics_system.h"
#include "systems/rendering_system.h"
#include "utils/parameter_server.h"
#include "utils/random_number_generator.h"
#include "utils/rate_timer.h"
#include "utils/simple_profiler.h"

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
  std::unique_ptr<RenderingSystem> rendering_system_;
  std::unique_ptr<PhysicsSystem> physics_system_;
  std::shared_ptr<ParameterServer> parameter_server_;
  std::unique_ptr<InputProcessor> input_processor_;
  std::shared_ptr<SoundPlayer> sound_player_;
  std::shared_ptr<SoundProcessor> sound_processor_;
  std::shared_ptr<AnimationManager> animation_manager_;
  std::shared_ptr<RandomNumberGenerator> rng_;

  RateTimer rate_;
  SimpleProfiler profiler_;

  EntityId player_id_;

#ifdef _WIN32
  WindowsHighResTimer high_res_timer_{1};
#endif
};

}  // namespace platformer