#pragma once

#include <map>

#include "olcPixelGameEngine.h"

namespace platformer {

// All possible inputs.
enum class InputAction : uint8_t {
  Left,
  Right,
  Up,
  Down,
  Shoot,
  Jump,
  Roll,
  Backshot,
  Suicide,
  Quit,
  Menu,
  Console,
};

struct InputButton {
  bool pressed;
  bool held;
  bool released;
};

// Very simple class to wrap olc key input.
// This allows easy customization of key bindings,
// or multiple bindings for an action (e.g. controller support)
class InputCapture {
 public:
  InputCapture(olc::PixelGameEngine* engine_ptr);

  void Capture();

  [[nodiscard]] const InputButton& GetKey(InputAction action) const;

 private:
  olc::PixelGameEngine* engine_ptr_;
  std::map<olc::Key, InputAction> bindings_;
  std::map<InputAction, InputButton> input_buttons_;
};

}  // namespace platformer