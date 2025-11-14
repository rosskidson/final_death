#pragma once

#include <string>

#include "utils/game_clock.h"

namespace platformer {

enum class Actor : uint8_t { Player, Enemy, Boss };

inline std::string ToString(Actor actor) {
  switch (actor) {
    case Actor::Player:
      return "Player";
    case Actor::Enemy:
      return "Enemy";
    case Actor::Boss:
      return "Boss";
  }
  return "unknown actor";
}

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

// class StateAccess {
//  public:
//   StateAccess() = default;
//   StateAccess(State state) : state_{state} {}
//   [[nodiscard]] State GetState() const { return state_; }
//   void SetState(State state ) {
//     if (state_ == state) {
//       return;
//     }
//     state_set_at_ = GameClock::NowGlobal();
//     state_ = state;
//   }

//   [[nodiscard]] TimePoint GetStateSetAt() const { return state_set_at_; }

//  private:
//   State state_{State::Idle};
//   TimePoint state_set_at_{GameClock::NowGlobal()};
// };

}  // namespace platformer