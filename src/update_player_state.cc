#include "update_player_state.h"

#include <chrono>

#include "actor_state.h"
#include "components.h"
#include "input_capture.h"
#include "physics_engine.h"
#include "registry.h"
#include "registry_helpers.h"
#include "utils/chrono_helpers.h"
#include "utils/game_clock.h"
#include "utils/logging.h"
#include "utils/parameter_server.h"

namespace platformer {

#define TRY_SET_STATE(requested_states, state_component, new_state) \
  do {                                                              \
    if ((requested_states).count(new_state)) {                      \
      (state_component).state.SetState(new_state);                  \
      return;                                                       \
    }                                                               \
  } while (0)

#define LATCH_STATE(state, STATE)                                                                \
  do {                                                                                           \
    if ((state).state == (STATE) && !(state).animation_manager.GetActiveAnimation().Expired()) { \
      return;                                                                                    \
    }                                                                                            \
  } while (0)

namespace {

bool IsInterruptibleState(State state) {
  switch (state) {
    case State::Shoot:
    case State::UpShot:
    case State::BackShot:
    case State::BackDodgeShot:
    case State::InAirShot:
    case State::InAirDownShot:
    case State::CrouchShot:
    case State::PreRoll:
    case State::Roll:
    case State::PreJump:
    case State::HardLanding:
    case State::Suicide:
      return false;
  }
  return true;
}
bool MovementDisallowed(State state) {
  switch (state) {
    case State::Shoot:
    case State::UpShot:
    case State::BackShot:
    case State::AimUp:
    case State::CrouchShot:
    case State::HardLanding:
    case State::PreSuicide:
    case State::Suicide:
      return true;
  }
  return false;
}

bool Shooting(const State state) {
  return state == State::Shoot || state == State::CrouchShot || state == State::InAirShot;
}

bool Squish(const AxisCollisions& collisions) {
  return collisions.lower_collision && collisions.upper_collision;
}

State GetShootState(const std::set<State>& requested_states,
                    const StateAccess& state,
                    const Collision& collisions) {
  if (state.GetState() == State::PreSuicide) {
    return State::Suicide;
  }
  if (!collisions.bottom) {
    if (requested_states.count(State::Crouch) != 0) {
      return State::InAirDownShot;
    }
    return State::InAirShot;
  }
  if (state.GetState() == State::AimUp || state.GetState() == State::UpShot) {
    return State::UpShot;
  }
  if (requested_states.count(State::Crouch) != 0) {
    return State::CrouchShot;
  }
  return State::Shoot;
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
//       state.state = State::HardLanding;
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
    if ((player.state == State::InAirShot || player.state == State::InAirDownShot) &&
        player.collisions.bottom &&
        (ToMs(GameClock::NowGlobal() -
              player.animation_manager.GetActiveAnimation().GetStartTime()) > 300)) {
      player.animation_manager.SwapAnimation(State::Shoot);
      player.state = State::Shoot;
    }

    // Transition from Roll to PostRoll
    // TODO:: ints in parameter server
    const int roll_duration_ms =
        static_cast<int>(parameter_server.GetParameter<double>("timing/roll.duration.ms"));
    if (player.state == State::Roll && GameClock::NowGlobal() - player.roll_start_time >
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
        player.state = State::PostRoll;
      }
    }

    if (player.state == State::Roll && player.requested_states.count(State::BackShot)) {
      player.facing = player.facing == Direction::LEFT ? Direction::RIGHT : Direction::LEFT;
      player.state = State::BackDodgeShot;
      return;
    }
    return;
  }
  if (player.requested_states.count(State::Shoot)) {
    player.state = GetShootState(player);
    return;
  }

  // Shoot backwards only when standing or walking.
  if (player.requested_states.count(State::BackShot) &&
      (player.state == State::Walk || player.state == State::Idle)) {
    player.state = State::BackShot;
    return;
  }

  // BackDodgeShot only from crouching state.
  if ((player.state == State::Crouch || player.state == State::CrouchShot) &&
      player.requested_states.count(State::Crouch) &&
      player.requested_states.count(State::BackShot)) {
    player.state = State::BackDodgeShot;
    return;
  }

  // Transition from PreRoll to Roll
  if (player.state == State::PreRoll &&
      player.animation_manager.GetActiveAnimation().Expired()) {
    player.roll_start_time = GameClock::NowGlobal();
    player.state = State::Roll;
    return;
  }

  // Remaining non-interruptable states
  TRY_SET_STATE(player, State::PreRoll);

  // Next is to latch some non interruptable states.
  LATCH_STATE(player, State::PostRoll);
  LATCH_STATE(player, State::PreSuicide);

  // InAir has priority over other states.
  if (!player.collisions.bottom) {
    if (player.requested_states.count(State::InAirDownShot)) {
      player.state = State::InAirDownShot;
    } else {
      player.state = State::InAir;
    }
    return;
  }

  // Lower priority interruptable states.
  TRY_SET_STATE(player, State::PreJump);
  TRY_SET_STATE(player, State::Crouch);
  TRY_SET_STATE(player, State::AimUp);
  TRY_SET_STATE(player, State::PreSuicide);
  TRY_SET_STATE(player, State::Walk);

  // Soft landing may be interrupted by anything : lowest priority.
  if (player.collisions.bottom && player.collisions.bottom_changed) {
    player.state = State::SoftLanding;
    return;
  }
  LATCH_STATE(player, State::SoftLanding);
  */
  auto [player, state] = registry.GetComponents<PlayerComponent, StateComponent>(player_id);

  for (int i = 0; i < static_cast<int>(State::Dead) + 1; ++i) {
    const auto set_state = static_cast<State>(i);
    if (IsInterruptibleState(set_state)) {
      continue;
    }
    TRY_SET_STATE(player.requested_states, state, set_state);
  }
  for (int i = 0; i < static_cast<int>(State::Dead) + 1; ++i) {
    const auto set_state = static_cast<State>(i);
    if (!IsInterruptibleState(set_state)) {
      continue;
    }
    TRY_SET_STATE(player.requested_states, state, set_state);
  }
}

}  // namespace

void UpdateMaxVelocity(const ParameterServer& parameter_server, Registry& registry) {
  const double walk_x = parameter_server.GetParameter<double>("physics/max.x.vel");
  const double walk_y = parameter_server.GetParameter<double>("physics/max.y.vel");
  const double slide_x = parameter_server.GetParameter<double>("physics/slide.x.vel");
  const double roll_x = parameter_server.GetParameter<double>("physics/roll.x.vel");
  for (auto id : registry.GetView<Velocity, StateComponent>()) {
    auto [velocity, state] = registry.GetComponents<Velocity, StateComponent>(id);
    if (state.state.GetState() == State::Roll) {
      velocity.max_x = roll_x;
    } else if (state.state.GetState() == State::BackDodgeShot) {
      velocity.max_x = slide_x;
    } else {
      velocity.max_x = walk_x;
    }
    velocity.max_y = walk_y;
  }
}

void UpdateComponentsFromState(const ParameterServer& parameter_server, Registry& registry) {
  UpdateMaxVelocity(parameter_server, registry);
}
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

void UpdateState(const ParameterServer& parameter_server, EntityId player_id, Registry& registry) {
  UpdateStateImpl(parameter_server, player_id, registry);
  auto [state] = registry.GetComponents<PlayerComponent>(player_id);
  state.requested_states.clear();
}

}  // namespace platformer