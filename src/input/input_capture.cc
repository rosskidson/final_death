#include "input/input_capture.h"

namespace platformer {

InputCapture::InputCapture(olc::PixelGameEngine* engine_ptr) : engine_ptr_{engine_ptr} {
  // TODO:: Load this via some sort of config.
  bindings_[olc::Key::LEFT] = InputAction::Left;
  bindings_[olc::Key::RIGHT] = InputAction::Right;
  bindings_[olc::Key::UP] = InputAction::Up;
  bindings_[olc::Key::DOWN] = InputAction::Down;
  bindings_[olc::Key::CTRL] = InputAction::Shoot;
  bindings_[olc::Key::SPACE] = InputAction::Jump;
  bindings_[olc::Key::SHIFT] = InputAction::Roll;
  bindings_[olc::Key::Z] = InputAction::Backshot;
  bindings_[olc::Key::S] = InputAction::Suicide;
  bindings_[olc::Key::Q] = InputAction::Quit;
  bindings_[olc::Key::ESCAPE] = InputAction::Menu;
  bindings_[olc::Key::TAB] = InputAction::Console;
}

void InputCapture::Capture() {
  // Reset all input
  input_buttons_.clear();
  for (const auto& [key, action] : bindings_) {
    auto& button = input_buttons_[action];
    // Use or in case there are multiple bindings.
    button.held |= engine_ptr_->GetKey(key).bHeld;
    button.pressed |= engine_ptr_->GetKey(key).bPressed;
    button.released |= engine_ptr_->GetKey(key).bReleased;
  }
}

const InputButton& InputCapture::GetKey(InputAction action) const {
  return input_buttons_.at(action);
}

};  // namespace platformer