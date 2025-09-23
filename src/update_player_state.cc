#include "update_player_state.h"

#include "player_state.h"
#include "utils/game_clock.h"

namespace platformer {

namespace {

bool IsInterruptibleState(PlayerState state) {
  switch (state) {
    case PlayerState::Shoot:
    case PlayerState::InAirShoot:
    case PlayerState::CrouchShoot:
    case PlayerState::PreRoll:
    case PlayerState::Roll:
    case PlayerState::PreJump:
    case PlayerState::Suicide:
      return false;
  }
  return true;
}

bool Shooting(const PlayerState state) {
  return state == PlayerState::Shoot || state == PlayerState::CrouchShoot ||
         state == PlayerState::InAirShoot;
}

void UpdateStateImpl(Player& player) {
  if (!player.animation_manager.GetActiveAnimation().Expired() &&
      !IsInterruptibleState(player.state)) {
    // If the player is shooting in the air and lands, transition the animation to the standing
    // pose.  Only do this after the first few frame to avoid double fire.
    if (player.state == PlayerState::InAirShoot && player.collisions.bottom &&
        ((GameClock::NowGlobal() - player.animation_manager.GetActiveAnimation().GetStartTime())
                 .count() /
             1e6 >
         100)) {
      player.animation_manager.SwapAnimation(PlayerState::Shoot);
      player.state = PlayerState::Shoot;
    }
    return;
  }
  if (player.requested_states.count(PlayerState::Shoot)) {
    if (!player.collisions.bottom) {
      player.state = PlayerState::InAirShoot;
    } else if (player.requested_states.count(PlayerState::Crouch)) {
      player.state = PlayerState::CrouchShoot;
    } else {
      player.state = PlayerState::Shoot;
    }
    return;
  }

  // Transition from PreRoll to Roll
  if (player.state == PlayerState::PreRoll &&
      player.animation_manager.GetActiveAnimation().Expired()) {
    player.state = PlayerState::Roll;
    return;
  }

  // Remaining non-interruptable states
  std::vector<PlayerState> non_interruptable_states = {PlayerState::PreRoll};
  for (const auto& state : non_interruptable_states) {
    if (player.requested_states.count(state)) {
      player.state = state;
      return;
    }
  }

  // InAir has priority over other states.
  if (!player.collisions.bottom) {
    player.state = PlayerState::InAir;
    return;
  }

  // Lower priority interruptable states.
  std::vector<PlayerState> lower_priority_states = {PlayerState::PreJump, PlayerState::Crouch,
                                                    PlayerState::Walk, PlayerState::PreSuicide};
  for (const auto& state : lower_priority_states) {
    if (player.requested_states.count(state)) {
      player.state = state;
      return;
    }
  }
  player.state = PlayerState::Idle;
}

}  // namespace

void UpdatePlayerFromState(Player& player) {
  // Disallow movement during firing.
  if ((player.state == PlayerState::Shoot || player.state == PlayerState::CrouchShoot) &&
      !player.animation_manager.GetActiveAnimation().Expired()) {
    player.velocity.x = 0;
    player.acceleration.x = 0;
  }

  // Disallow movement during crouch, except changing direction.
  if (player.state == PlayerState::Crouch) {
    if (player.facing_left && player.acceleration.x > 0) {
      player.velocity.x = 1;
    } else if (!player.facing_left && player.acceleration.x < 0) {
      player.velocity.x = -1;
    } else {
      player.velocity.x = 0;
    }
    player.acceleration.x = 0;
  }
}

void UpdateState(Player& player) {
  UpdateStateImpl(player);
  player.requested_states.clear();
}

}  // namespace platformer