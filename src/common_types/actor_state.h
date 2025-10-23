#pragma once

#include <string>

#include "utils/game_clock.h"

namespace platformer {

enum class Actor : uint8_t { Player, Enemy, BadassEnemy, Boss };

enum class State {
  Idle,
  Walk,
  Shoot,
  PreJump,
  HardLanding,
  SoftLanding,
  InAir,
  InAirShot,
  InAirAimDown,
  InAirDownShot,
  AimUp,
  UpShot,
  BackShot,
  BackDodgeShot,
  Crouch,
  CrouchShot,
  PreRoll,
  Roll,
  PostRoll,
  PreSuicide,
  Suicide,
  Dying,
  Dead
};

inline std::string ToString(State state) {
  switch (state) {
    case State::Idle:
      return "Idle";
    case State::Walk:
      return "Walk";
    case State::Shoot:
      return "Shoot";
    case State::PreJump:
      return "PreJump";
    case State::HardLanding:
      return "HardLanding";
    case State::SoftLanding:
      return "SoftLanding";
    case State::InAir:
      return "InAir";
    case State::InAirShot:
      return "InAirShot";
    case State::InAirAimDown:
      return "InAirAimDown";
    case State::InAirDownShot:
      return "InAirDownShot";
    case State::AimUp:
      return "AimUp";
    case State::UpShot:
      return "UpShot";
    case State::BackShot:
      return "BackShot";
    case State::BackDodgeShot:
      return "BackDodgeShot";
    case State::Crouch:
      return "Crouch";
    case State::CrouchShot:
      return "CrouchShot";
    case State::PreRoll:
      return "PreRoll";
    case State::Roll:
      return "Roll";
    case State::PostRoll:
      return "PostRoll";
    case State::PreSuicide:
      return "PreSuicide";
    case State::Suicide:
      return "Suicide";
    case State::Dying:
      return "Dying";
    case State::Dead:
      return "Dead";
  }
  return "unknown state";
}

class StateAccess {
 public:
  StateAccess() = default;
  StateAccess(State state) : state_{state} {}
  [[nodiscard]] State GetState() const { return state_; }
  void SetState(State state, bool reset = false) {
    if (state_ == state && !reset) {
      return;
    }
    state_set_at_ = GameClock::NowGlobal();
    last_animation_frame_idx_ = -1;
    state_ = state;
  }

  [[nodiscard]] TimePoint GetStateSetAt() const { return state_set_at_; }

  [[nodiscard]] int GetLastAnimationFrameIdx() const { return last_animation_frame_idx_; }
  void SetLastAnimationFrameIdx(int last_animation_frame_idx) {
    last_animation_frame_idx_ = last_animation_frame_idx;
  }

 private:
  State state_{State::Idle};
  TimePoint state_set_at_{GameClock::NowGlobal()};

  // I know this is not excellent that this is in here, but it needs to be reset on state changes
  // This could be fixed by creating an animation state component that contains it's
  // own state, but then it needs to be explicitly updated once per game loop.
  int last_animation_frame_idx_{-1};
};

}  // namespace platformer