#pragma once

//////////////////////////////////////////////////////////////////////////////
//  THIS HEADER IS CURRENTLY UNUSED.
//
// This is an alternate implementation of state, where the there is a different state enum for every
// actor. There is a 'common' state enum for shared state among all actors.  There is also an
// interface class that stores one state value.  This allows multiple pointers to the state: common
// state and specialized state. The idea is to have a CommonState component and a Specialized (e.g.
// PlayerState) component, that both provide access to state, but there is still only one underlying
// state value. That way systems can query either the common state or the specialized state as
// needed.
//
// This was ditched due to complexity, and usage overhead, and it also obscures somewhat where the
// state is stored when it is shared across multiple components.  The header has not been removed
// (yet), as it may still be useful in the future.

#include <string>

#include "utils/game_clock.h"

namespace platformer {

enum class Actor : uint8_t { Player, Enemy, BadassEnemy, Boss };

enum class CommonState : uint8_t { Idle, Walk, Shoot, InAir, Dying, Dead, SpecializedState, SIZE };

enum class PlayerState : uint8_t {
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
  if (static_cast<CommonState>(state) < CommonState::SIZE) {
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
    if (static_cast<CommonState>(state_) >= CommonState::SpecializedState) {
      return CommonState::SpecializedState;
    }
    return static_cast<CommonState>(state_);
  }
  void SetCommonState(CommonState state) { SetState(static_cast<uint8_t>(state)); }

  void SetState(uint8_t state) {
    if (state_ == state) {
      return;
    }
    state_set_at_ = GameClock::NowGlobal();
    last_animation_frame_idx = -1;
    state_ = state;
  }

  TimePoint GetStateSetAt() const { return state_set_at_; }

  int GetLastAnimationFrame() const { return last_animation_frame_idx; }

  [[nodiscard]] uint8_t GetTypeErasedState() { return state_; }

 protected:
  uint8_t state_{static_cast<uint8_t>(CommonState::Idle)};
  TimePoint state_set_at_;

  // I know this is not excellent that this is in here, but it needs to be reset on state changes
  // This could be fixed by creating an animation state component that contains it's
  // own state, but then it needs to be explicitly updated once per game loop.
  int last_animation_frame_idx{-1};
};

class PlayerStateAccess : public StateInterface {
 public:
  PlayerStateAccess() { SetState(PlayerState::Idle); }
  ~PlayerStateAccess() override = default;

  using StateInterface::SetState;

  [[nodiscard]] PlayerState GetState() const { return static_cast<PlayerState>(this->state_); }
  void SetState(PlayerState state) { this->SetState(static_cast<uint8_t>(state)); }
};

}  // namespace platformer