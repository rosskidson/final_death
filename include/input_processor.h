#pragma once

#include <memory>

#include "basic_types.h"
#include "input_capture.h"
#include "olcPixelGameEngine.h"
#include "registry.h"
#include "registry_helpers.h"
#include "sound.h"
#include "utils/parameter_server.h"

namespace platformer {

class InputProcessor {
 public:
  InputProcessor(std::shared_ptr<ParameterServer> parameter_server,
                 std::shared_ptr<const SoundPlayer> sound_player,
                 std::shared_ptr<Registry> registry,
                 olc::PixelGameEngine* engine_ptr);

  bool ProcessInputs(EntityId player_id);

 private:
  olc::PixelGameEngine* engine_ptr_;
  InputCapture input_;
  std::shared_ptr<ParameterServer> parameter_server_;
  std::shared_ptr<const SoundPlayer> sound_player_;
  std::shared_ptr<Registry> registry_;
};

}  // namespace platformer