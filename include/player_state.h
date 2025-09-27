#pragma once

#include <string>

namespace platformer {

enum class PlayerState {
  Idle,
  Walk,
  Shoot,
  PreJump,
  Landing,
  InAir,
  InAirShoot,
  InAirDownShoot,
  AimUp,
  ShootUp,
  Crouch,
  CrouchShoot,
  PreRoll,
  Roll,
  PostRoll,
  PreSuicide,
  Suicide,
  Dying,
  Dead
};



inline std::string ToString(PlayerState state) {
  switch (state) {
    case PlayerState::Idle:
      return "Idle";
    case PlayerState::Walk:
      return "Walk";
    case PlayerState::Shoot:
      return "Shoot";
    case PlayerState::PreJump:
      return "PreJump";
    case PlayerState::Landing:
      return "Landing";
    case PlayerState::InAir:
      return "InAir";
    case PlayerState::InAirShoot:
      return "InAirShoot";
    case PlayerState::InAirDownShoot:
      return "InAirDownShoot";
    case PlayerState::AimUp:
      return "AimUp";
    case PlayerState::ShootUp:
      return "ShootUp";
    case PlayerState::Crouch:
      return "Crouch";
    case PlayerState::CrouchShoot:
      return "CrouchShoot";
    case PlayerState::PreRoll:
      return "PreRoll";
    case PlayerState::Roll:
      return "Roll";
    case PlayerState::PostRoll:
      return "PostRoll";
    case PlayerState::PreSuicide:
      return "PreSuicide";
    case PlayerState::Suicide:
      return "Suicide";
    case PlayerState::Dying:
      return "Dying";
    case PlayerState::Dead:
      return "Dead";
  }
  return "unknown state";
}

}