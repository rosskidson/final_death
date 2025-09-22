#pragma once

namespace platformer {

enum class PlayerState {
  Idle,
  Walk,
  Shoot,
  PreJump,
  InAir,
  InAirShoot,
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

}