#pragma once

#include <string>

namespace platformer {

enum class Actor : uint8_t { Player, Enemy, BadassEnemy, Boss };

enum class CommonState : uint8_t {
  Idle,
  Walk,
  Shoot,
  InAir,
  Dying,
  Dead,
  SpecializedState,
  SIZE
};

enum class PlayerState : uint8_t{
  Idle = static_cast<uint8_t>(CommonState::Idle),
  Walk = static_cast<uint8_t>(CommonState::Walk),
  Shoot = static_cast<uint8_t>(CommonState::Shoot),
  InAir = static_cast<uint8_t>(CommonState::InAir),
  Dying = static_cast<uint8_t>(CommonState::Dying),
  Dead = static_cast<uint8_t>(CommonState::Dead),
  PreJump,
  HardLanding,
  SoftLanding,
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
  Suicide
};


inline std::string ToString(CommonState state) {
  switch (state) {
    case CommonState::Idle:
      return "Idle";
    case CommonState::Walk:
      return "Walk";
    case CommonState::Shoot:
      return "Shoot";
    case CommonState::InAir:
      return "InAir";
    case CommonState::Dying:
      return "Dying";
    case CommonState::Dead:
      return "Dead";
    case CommonState::SpecializedState:
      return "SpecializedState";
  }
  return "Unknown state";
}

inline std::string ToString(PlayerState state) {
  if(static_cast<CommonState>(state) < CommonState::SIZE) {
    return ToString(static_cast<CommonState>(state));
  }
  switch (state) {
    case PlayerState::PreJump:
      return "PreJump";
    case PlayerState::HardLanding:
      return "HardLanding";
    case PlayerState::SoftLanding:
      return "SoftLanding";
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
  }
  return "Unknown state";
}

class StateInterface {
 public:
  virtual ~StateInterface() = default;
  [[nodiscard]] CommonState GetCommonState() const { 
    if(state_ >= CommonState::SpecializedState) {
      return CommonState::SpecializedState;
    }
    return state_; 
  }
  void SetCommonState(CommonState state) { state_ = state; }

  [[nodiscard]] uint8_t GetTypeErasedState() { return state_; }

 protected:
  uint8_t state_{static_cast<uint8_t>(CommonState::Idle)};
};

class PlayerStateAccess : public StateInterface {
 public:
  PlayerStateAccess() = default;
  ~PlayerStateAccess() override = default;

  [[nodiscard]] PlayerState GetState() const { return state_; }
  void SetState(PlayerState state) { this->state_ = static_cast<uint8_t>(state); }
};

}