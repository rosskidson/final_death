#pragma once

#include <memory>

#include "basic_types.h"
#include "input_capture.h"
#include "olcPixelGameEngine.h"
#include "utils/parameter_server.h"

namespace platformer {

class InputProcessor {
 public:
  InputProcessor(std::shared_ptr<const ParameterServer> parameter_server,
                 olc::PixelGameEngine* engine_ptr);

  bool ProcessInputs(Player& player);

 private:
  bool IsPlayerShooting(const Player& player);

  olc::PixelGameEngine* engine_ptr_;
  InputCapture input_;
  std::shared_ptr<const ParameterServer> parameter_server_;
};

}  // namespace platformer