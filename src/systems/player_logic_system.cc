#include "player_logic_system.h"

#include <algorithm>
#include <chrono>

#include "animation/animation_event.h"
#include "common_types/actor_state.h"
#include "common_types/components.h"
#include "registry.h"
#include "systems/physics_system.h"
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
                    const State& state,
                    const Collision& collisions) {
  if (state == State::PreSuicide) {
    return State::Suicide;
  }
  if (!collisions.bottom) {
    if (requested_states.count(State::Crouch) != 0) {
      return State::InAirDownShot;
    }
    return State::InAirShot;
  }
  if (state == State::AimUp || state == State::UpShot) {
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

bool AnimationExpired(const State state, const std::vector<AnimationEvent>& animation_events) {
  return std::any_of(
      animation_events.begin(), animation_events.end(), [&](const AnimationEvent& event) {
        return event.event_name == "AnimationEnded" && event.animation_state == state;
      });
}

}  // namespace

void UpdateState(const ParameterServer& parameter_server,
                 const std::vector<AnimationEvent>& animation_events,
                 const PhysicsSystem& physics_system,
                 EntityId player_id,
                 Registry& registry) {
  auto& state_component = registry.GetComponent<StateComponent>(player_id);
  const auto& state = state_component.state.GetState();
  const auto& collisions = registry.GetComponent<Collision>(player_id);
  const auto& requested_states = registry.GetComponent<PlayerComponent>(player_id).requested_states;
  const bool animation_expired = AnimationExpired(state, animation_events);

  // There is one thing that can interrupt non interuptable actions: Hard falling.
  // if (SetHardLandingState(parameter_server, player_id, registry)) {
  //   return;
  // }

  // Now check for non interruptable actions.
  if (!animation_expired && !IsInterruptibleState(state)) {
    // If the player is shooting in the air and lands, transition the animation to the standing
    // pose.  Only do this after the first few frame to avoid double fire.
    if ((state == State::InAirShot || state == State::InAirDownShot) && collisions.bottom &&
        (ToMs(GameClock::NowGlobal() - state_component.state.GetStateSetAt()) > 300)) {
      // player.animation_manager.SwapAnimation(State::Shoot);
      // TODO:: set setstarttime
      state_component.state.SetState(State::Shoot);
      return;
    }

    // Transition from Roll to PostRoll
    // TODO:: ints in parameter server
    const int roll_duration_ms =
        static_cast<int>(parameter_server.GetParameter<double>("timing/roll.duration.ms"));
    if (state == State::Roll && GameClock::NowGlobal() - state_component.state.GetStateSetAt() >
                                    std::chrono::milliseconds(roll_duration_ms)) {
      const auto& position = registry.GetComponent<Position>(player_id);
      auto collision_box = registry.GetComponent<CollisionBox>(player_id);
      collision_box.x_offset_px = 30;
      collision_box.y_offset_px = 0;
      collision_box.collision_width_px = 18;
      collision_box.collision_height_px = 48;
      AxisCollisions collisions_x =
          physics_system.CheckAxisCollision(position, collision_box, Axis::X);
      AxisCollisions collisions_y =
          physics_system.CheckAxisCollision(position, collision_box, Axis::Y);
      if (!Squish(collisions_x) && !Squish(collisions_y)) {
        state_component.state.SetState(State::PostRoll);
      }
      return;
    }

    if (state == State::Roll && requested_states.count(State::BackShot)) {
      auto& facing = registry.GetComponent<FacingDirection>(player_id).facing;
      facing = facing == Direction::LEFT ? Direction::RIGHT : Direction::LEFT;
      state_component.state.SetState(State::BackDodgeShot);
      return;
    }
    return;
  }
  if (requested_states.count(State::Shoot)) {
    state_component.state.SetState(GetShootState(requested_states, state, collisions),
                                   animation_expired);
    return;
  }

  /*
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
    */

  // Jump: Move to update from state!
  if (animation_expired && state == State::PreJump) {
    const auto jump_velocity = parameter_server.GetParameter<double>("physics/jump.velocity");
    registry.GetComponent<Velocity>(player_id).y = jump_velocity;
  }

  // InAir has priority over other states.
  if (!collisions.bottom) {
    if (requested_states.count(State::InAirDownShot)) {
      state_component.state.SetState(State::InAirDownShot);
    } else {
      state_component.state.SetState(State::InAir);
    }
    return;
  }

  // Lower priority interruptable states.
  TRY_SET_STATE(requested_states, state_component, State::PreJump);
  TRY_SET_STATE(requested_states, state_component, State::Crouch);
  TRY_SET_STATE(requested_states, state_component, State::AimUp);
  TRY_SET_STATE(requested_states, state_component, State::PreSuicide);
  TRY_SET_STATE(requested_states, state_component, State::Walk);

  // Soft landing may be interrupted by anything : lowest priority.
  // if (player.collisions.bottom && player.collisions.bottom_changed) {
  //   player.state = State::SoftLanding;
  //   return;
  // }
  // LATCH_STATE(player, State::SoftLanding);

  state_component.state.SetState(State::Idle);
}

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

// void UpdateState(const ParameterServer& parameter_server, EntityId player_id, Registry& registry)
// {
//   UpdateStateImpl(parameter_server, player_id, registry);
//   auto [state] = registry.GetComponents<PlayerComponent>(player_id);
//   state.requested_states.clear();
// }

}  // namespace platformer