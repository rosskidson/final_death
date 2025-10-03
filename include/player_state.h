#pragma once

#include <string>

namespace platformer {

enum class PlayerState {
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
    case PlayerState::HardLanding:
      return "HardLanding";
    case PlayerState::SoftLanding:
      return "SoftLanding";
    case PlayerState::InAir:
      return "InAir";
    case PlayerState::InAirShot:
      return "InAirShot";
    case PlayerState::InAirAimDown:
      return "InAirAimDown";
    case PlayerState::InAirDownShot:
      return "InAirDownShot";
    case PlayerState::AimUp:
      return "AimUp";
    case PlayerState::UpShot:
      return "UpShot";
    case PlayerState::BackShot:
      return "BackShot";
    case PlayerState::BackDodgeShot:
      return "BackDodgeShot";
    case PlayerState::Crouch:
      return "Crouch";
    case PlayerState::CrouchShot:
      return "CrouchShot";
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