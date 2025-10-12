#include "update_player_state.h"

#include <chrono>

#include "components.h"
#include "input_capture.h"
#include "physics_engine.h"
#include "player_state.h"
#include "registry.h"
#include "registry_helpers.h"
#include "utils/chrono_helpers.h"
#include "utils/game_clock.h"
#include "utils/logging.h"
#include "utils/parameter_server.h"

namespace platformer {

#define TRY_SET_STATE(state, STATE)              \
  do {                                           \
    if ((state).requested_states.count(STATE)) { \
      (state).state = (STATE);                   \
      return;                                    \
    }                                            \
  } while (0)

#define LATCH_STATE(state, STATE)                                                                \
  do {                                                                                           \
    if ((state).state == (STATE) && !(state).animation_manager.GetActiveAnimation().Expired()) { \
      return;                                                                                    \
    }                                                                                            \
  } while (0)

namespace {

bool IsInterruptibleState(PlayerState state) {
  switch (state) {
    case PlayerState::Shoot:
    case PlayerState::UpShot:
    case PlayerState::BackShot:
    case PlayerState::BackDodgeShot:
    case PlayerState::InAirShot:
    case PlayerState::InAirDownShot:
    case PlayerState::CrouchShot:
    case PlayerState::PreRoll:
    case PlayerState::Roll:
    case PlayerState::PreJump:
    case PlayerState::HardLanding:
    case PlayerState::Suicide:
      return false;
  }
  return true;
}
bool MovementDisallowed(PlayerState state) {
  switch (state) {
    case PlayerState::Shoot:
    case PlayerState::UpShot:
    case PlayerState::BackShot:
    case PlayerState::AimUp:
    case PlayerState::CrouchShot:
    case PlayerState::HardLanding:
    case PlayerState::PreSuicide:
    case PlayerState::Suicide:
      return true;
  }
  return false;
}

bool Shooting(const PlayerState state) {
  return state == PlayerState::Shoot || state == PlayerState::CrouchShot ||
         state == PlayerState::InAirShot;
}

bool Squish(const AxisCollisions& collisions) {
  return collisions.lower_collision && collisions.upper_collision;
}

PlayerState GetShootState(const State& state, const Collision& collisions) {
  if (state.state == PlayerState::PreSuicide) {
    return PlayerState::Suicide;
  }
  if (!collisions.bottom) {
    if (state.requested_states.count(PlayerState::Crouch)) {
      return PlayerState::InAirDownShot;
    }
    return PlayerState::InAirShot;
  }
  if (state.state == PlayerState::AimUp || state.state == PlayerState::UpShot) {
    return PlayerState::UpShot;
  }
  if (state.requested_states.count(PlayerState::Crouch)) {
    return PlayerState::CrouchShot;
  }
  return PlayerState::Shoot;
}

// bool SetHardLandingState(const ParameterServer& parameter_server,
//                          EntityId player_id,
//                          Registry& registry) {
//   auto [velocity, collisions, state, distance_fallen] =
//       registry.GetComponents<Velocity, Collision, State, DistanceFallen>(player_id);
//   const auto hard_fall_distance =
//       parameter_server.GetParameter<double>("physics/hard.fall.distance");
//   if (collisions.bottom) {
//     if (distance_fallen.distance_fallen > hard_fall_distance) {
//       distance_fallen.distance_fallen = 0;
//       state.state = PlayerState::HardLanding;
//       return true;
//     }
//   }
//   if (velocity.y > 0) {
//     distance_fallen.distance_fallen = 0;
//     return false;
//   }
//   return false;
// }

void UpdateStateImpl(const ParameterServer& parameter_server,
                     EntityId player_id,
                     Registry& registry) {
  // There is one thing that can interrupt non interuptable actions: Hard falling.
  // if (SetHardLandingState(parameter_server, player_id, registry)) {
  //   return;
  // }

  /*
  // Now check for non interruptable actions.
  if (!player.animation_manager.GetActiveAnimation().Expired() &&
      !IsInterruptibleState(player.state)) {
    // If the player is shooting in the air and lands, transition the animation to the standing
    // pose.  Only do this after the first few frame to avoid double fire.
    if ((player.state == PlayerState::InAirShot || player.state == PlayerState::InAirDownShot) &&
        player.collisions.bottom &&
        (ToMs(GameClock::NowGlobal() -
              player.animation_manager.GetActiveAnimation().GetStartTime()) > 300)) {
      player.animation_manager.SwapAnimation(PlayerState::Shoot);
      player.state = PlayerState::Shoot;
    }

    // Transition from Roll to PostRoll
    // TODO:: ints in parameter server
    const int roll_duration_ms =
        static_cast<int>(parameter_server.GetParameter<double>("timing/roll.duration.ms"));
    if (player.state == PlayerState::Roll && GameClock::NowGlobal() - player.roll_start_time >
                                                 std::chrono::milliseconds(roll_duration_ms)) {
      // HACK: set the collisions back to full size dude to check for squishing.
      // TODO:: less hacky, copy the player, or supply a custom bounding box.
      player.x_offset_px = 30;
      player.y_offset_px = 0;
      player.collision_width_px = 18;
      player.collision_height_px = 48;
      AxisCollisions collisions_x = physics.CheckAxisCollision(player, Axis::X);
      AxisCollisions collisions_y = physics.CheckAxisCollision(player, Axis::Y);
      if (!Squish(collisions_x) && !Squish(collisions_y)) {
        player.state = PlayerState::PostRoll;
      }
    }

    if (player.state == PlayerState::Roll && player.requested_states.count(PlayerState::BackShot)) {
      player.facing = player.facing == Direction::LEFT ? Direction::RIGHT : Direction::LEFT;
      player.state = PlayerState::BackDodgeShot;
      return;
    }
    return;
  }
  if (player.requested_states.count(PlayerState::Shoot)) {
    player.state = GetShootState(player);
    return;
  }

  // Shoot backwards only when standing or walking.
  if (player.requested_states.count(PlayerState::BackShot) &&
      (player.state == PlayerState::Walk || player.state == PlayerState::Idle)) {
    player.state = PlayerState::BackShot;
    return;
  }

  // BackDodgeShot only from crouching state.
  if ((player.state == PlayerState::Crouch || player.state == PlayerState::CrouchShot) &&
      player.requested_states.count(PlayerState::Crouch) &&
      player.requested_states.count(PlayerState::BackShot)) {
    player.state = PlayerState::BackDodgeShot;
    return;
  }

  // Transition from PreRoll to Roll
  if (player.state == PlayerState::PreRoll &&
      player.animation_manager.GetActiveAnimation().Expired()) {
    player.roll_start_time = GameClock::NowGlobal();
    player.state = PlayerState::Roll;
    return;
  }

  // Remaining non-interruptable states
  TRY_SET_STATE(player, PlayerState::PreRoll);

  // Next is to latch some non interruptable states.
  LATCH_STATE(player, PlayerState::PostRoll);
  LATCH_STATE(player, PlayerState::PreSuicide);

  // InAir has priority over other states.
  if (!player.collisions.bottom) {
    if (player.requested_states.count(PlayerState::InAirDownShot)) {
      player.state = PlayerState::InAirDownShot;
    } else {
      player.state = PlayerState::InAir;
    }
    return;
  }

  // Lower priority interruptable states.
  TRY_SET_STATE(player, PlayerState::PreJump);
  TRY_SET_STATE(player, PlayerState::Crouch);
  TRY_SET_STATE(player, PlayerState::AimUp);
  TRY_SET_STATE(player, PlayerState::PreSuicide);
  TRY_SET_STATE(player, PlayerState::Walk);

  // Soft landing may be interrupted by anything : lowest priority.
  if (player.collisions.bottom && player.collisions.bottom_changed) {
    player.state = PlayerState::SoftLanding;
    return;
  }
  LATCH_STATE(player, PlayerState::SoftLanding);
  */
  auto& state = registry.GetComponent<State>(player_id);

  for (int i = 0; i < static_cast<int>(PlayerState::Dead) + 1; ++i) {
    const auto set_state = static_cast<PlayerState>(i);
    if(IsInterruptibleState(set_state)) {
      continue;
    }
    TRY_SET_STATE(state, set_state);
  }
  for (int i = 0; i < static_cast<int>(PlayerState::Dead) + 1; ++i) {
    const auto set_state = static_cast<PlayerState>(i);
    if(!IsInterruptibleState(set_state)) {
      continue;
    }
    TRY_SET_STATE(state, set_state);
  }

}

}  // namespace

// void UpdatePlayerFromState(const ParameterServer& parameter_server, Player& player) {
//   // Disallow movement for certain states.
//   if ((MovementDisallowed(player.state)) &&
//       !player.animation_manager.GetActiveAnimation().Expired()) {
//     player.velocity.x = 0;
//     player.acceleration.x = 0;
//   }

//   // Disallow movement during crouch, except changing direction.
//   if (player.state == PlayerState::Crouch) {
//     if (player.facing == Direction::LEFT && player.acceleration.x > 0) {
//       player.velocity.x = 1;
//     } else if (player.facing == Direction::RIGHT && player.acceleration.x < 0) {
//       player.velocity.x = -1;
//     } else {
//       player.velocity.x = 0;
//     }
//     player.acceleration.x = 0;
//   }

//   if (player.state == PlayerState::PreJump) {
//     if (player.velocity.x != 0) {
//       player.cached_velocity.x = player.velocity.x;
//     }
//     player.velocity.x = 0;
//     player.acceleration.x = 0;
//   }

//   // Roll
//   const auto roll_vel = parameter_server.GetParameter<double>("physics/roll.x.vel");
//   if (player.state == PlayerState::Roll) {
//     player.acceleration.x = 0;
//     if (player.facing == Direction::LEFT) {
//       player.velocity.x = -roll_vel;
//     } else {
//       player.velocity.x = roll_vel;
//     }
//     if ((player.collisions.left && player.acceleration.x > 0) ||
//         (player.collisions.right && player.acceleration.x < 0)) {
//       player.velocity.x *= -1;
//     }

//     // TODO:: This is hacky and should be configured better.
//     player.x_offset_px = 32;
//     player.y_offset_px = 0;
//     player.collision_width_px = 16;
//     player.collision_height_px = 16;
//   } else if (player.state == PlayerState::BackDodgeShot) {
//     player.x_offset_px = 20;
//     player.y_offset_px = 0;
//     player.collision_width_px = 30;
//     player.collision_height_px = 16;
//   } else {
//     player.x_offset_px = 30;
//     player.y_offset_px = 0;
//     player.collision_width_px = 18;
//     player.collision_height_px = 48;
//   }
// }

void UpdateState(const ParameterServer& parameter_server,
                 EntityId player_id,
                 Registry& registry) {
  UpdateStateImpl(parameter_server, player_id, registry);
  auto [state] = registry.GetComponents<State>(player_id);
  state.requested_states.clear();
}

}  // namespace platformer