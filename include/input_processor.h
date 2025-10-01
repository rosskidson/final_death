#pragma once

#include <memory>

#include "basic_types.h"
#include "input_capture.h"
#include "olcPixelGameEngine.h"
#include "player.h"
#include "sound.h"
#include "utils/parameter_server.h"

namespace platformer {

class InputProcessor {
 public:
  InputProcessor(std::shared_ptr<ParameterServer> parameter_server,
                 std::shared_ptr<const SoundPlayer> sound_player,
                 olc::PixelGameEngine* engine_ptr);

  bool ProcessInputs(Player& player);

 private:
  olc::PixelGameEngine* engine_ptr_;
  InputCapture input_;
  std::shared_ptr<ParameterServer> parameter_server_;
  std::shared_ptr<const SoundPlayer> sound_player_;
};

}  // namespace platformer