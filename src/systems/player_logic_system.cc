#include "player_logic_system.h"

#include <algorithm>
#include <chrono>

#include "animation/animation_event.h"
#include "common_types/actor_state.h"
#include "common_types/components.h"
#include "common_types/entity.h"
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

#define LATCH_STATE(state, expired, new_state) \
  do {                                         \
    if ((state) == (new_state) && !expired) {  \
      return;                                  \
    }                                          \
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

bool SetHardLandingState(const ParameterServer& parameter_server,
                         EntityId player_id,
                         Registry& registry) {
  auto [velocity, collisions, state_component, distance_fallen] =
      registry.GetComponents<Velocity, Collision, StateComponent, DistanceFallen>(player_id);
  const auto hard_fall_distance =
      parameter_server.GetParameter<double>("physics/hard.fall.distance");
  if (collisions.bottom) {
    if (distance_fallen.distance_fallen > hard_fall_distance) {
      distance_fallen.distance_fallen = 0;
      state_component.state.SetState(State::HardLanding);
      return true;
    }
  }
  if (velocity.y > 0) {
    distance_fallen.distance_fallen = 0;
    return false;
  }
  return false;
}

bool AnimationExpired(const State state, const std::vector<AnimationEvent>& animation_events) {
  return std::any_of(
      animation_events.begin(), animation_events.end(), [&](const AnimationEvent& event) {
        return event.event_name == "AnimationEnded" && event.animation_state == state;
      });
}

bool EventOccurred(const std::string& event_name,
                   const std::vector<AnimationEvent>& animation_events) {
  return std::any_of(animation_events.begin(), animation_events.end(),
                     [&](const AnimationEvent& event) { return event.event_name == event_name; });
}

void UpdatePlayerState(const EntityId player_id,
                       const ParameterServer& parameter_server,
                       const std::vector<AnimationEvent>& animation_events,
                       const PhysicsSystem& physics_system,
                       Registry& registry) {
  auto& state_component = registry.GetComponent<StateComponent>(player_id);
  const auto& state = state_component.state.GetState();
  const auto& collisions = registry.GetComponent<Collision>(player_id);
  const auto& requested_states = registry.GetComponent<PlayerComponent>(player_id).requested_states;
  const bool animation_expired = AnimationExpired(state, animation_events);

  // There is one thing that can interrupt non interuptable actions: Hard falling.
  if (SetHardLandingState(parameter_server, player_id, registry)) {
    return;
  }

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

  // Shoot backwards only when standing or walking.
  if (requested_states.count(State::BackShot) && (state == State::Walk || state == State::Idle)) {
    state_component.state.SetState(State::BackShot);
    return;
  }

  // BackDodgeShot only from crouching state.
  if ((state == State::Crouch || state == State::CrouchShot) &&
      requested_states.count(State::Crouch) && requested_states.count(State::BackShot)) {
    state_component.state.SetState(State::BackDodgeShot);
    return;
  }

  // Transition from PreRoll to Roll
  if (state == State::PreRoll && animation_expired) {
    state_component.state.SetState(State::Roll);
    return;
  }

  // Remaining non-interruptable states
  TRY_SET_STATE(requested_states, state_component, State::PreRoll);

  // Next is to latch some non interruptable states.
  LATCH_STATE(state, animation_expired, State::PostRoll);
  LATCH_STATE(state, animation_expired, State::PreSuicide);

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
  if (collisions.bottom && collisions.bottom_changed) {
    state_component.state.SetState(State::SoftLanding);
    return;
  }
  LATCH_STATE(state, animation_expired, State::SoftLanding);

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

void UpdatePlayerComponentsFromState(EntityId player_id,
                                     const ParameterServer& parameter_server,
                                     const std::vector<AnimationEvent>& animation_events,
                                     Registry& registry) {
  auto& state_component = registry.GetComponent<StateComponent>(player_id);
  auto& acceleration = registry.GetComponent<Acceleration>(player_id);
  auto& velocity = registry.GetComponent<Velocity>(player_id);
  auto& cached_velocity = registry.GetComponent<PlayerComponent>(player_id).cached_velocity;

  const auto& collisions = registry.GetComponent<Collision>(player_id);
  const auto& facing = registry.GetComponent<FacingDirection>(player_id).facing;

  const auto& state = state_component.state.GetState();
  const bool animation_expired = AnimationExpired(state, animation_events);

  // Disallow movement for certain states.
  if ((MovementDisallowed(state)) && !animation_expired) {
    velocity.x = 0;
    acceleration.x = 0;
  }

  // Disallow movement during crouch, except changing direction.
  if (state == State::Crouch) {
    if (facing == Direction::LEFT && acceleration.x > 0) {
      velocity.x = 1;
    } else if (facing == Direction::RIGHT && acceleration.x < 0) {
      velocity.x = -1;
    } else {
      velocity.x = 0;
    }
    acceleration.x = 0;
  }

  // Add vertical velocity for Jumping.
  if (std::any_of(
          animation_events.begin(), animation_events.end(), [](const AnimationEvent& event) {
            return event.event_name == "AnimationEnded" && event.animation_state == State::PreJump;
          })) {
    const auto jump_velocity = parameter_server.GetParameter<double>("physics/jump.velocity");
    velocity.x = cached_velocity.x;
    velocity.y = jump_velocity;
    cached_velocity.x = 0;
  }

  if (state == State::PreJump) {
    auto& cached_velocity = registry.GetComponent<PlayerComponent>(player_id).cached_velocity;
    if (velocity.x != 0) {
      cached_velocity.x = velocity.x;
    }
    velocity.x = 0;
    acceleration.x = 0;
  }

  if (EventOccurred("StartBackDodgeShot", animation_events)) {
    velocity.x = (facing == Direction::LEFT) ? 100 : -100;
  }

  if (EventOccurred("ShootShotgunDownInAir", animation_events)) {
    const auto shoot_down_upward_vel =
        parameter_server.GetParameter<double>("physics/shoot.down.upward.vel");
    velocity.y += shoot_down_upward_vel;
  }

  // Roll
  const auto roll_vel = parameter_server.GetParameter<double>("physics/roll.x.vel");
  if (state == State::Roll) {
    acceleration.x = 0;
    if (facing == Direction::LEFT) {
      velocity.x = -roll_vel;
    } else {
      velocity.x = roll_vel;
    }
    if ((collisions.left && acceleration.x > 0) || (collisions.right && acceleration.x < 0)) {
      velocity.x *= -1;
    }
  }

  // Set bounding box based on state
  // TODO:: This is hacky and should be configured better.
  auto& collision_box = registry.GetComponent<CollisionBox>(player_id);
  if (state == State::Roll) {
    collision_box.x_offset_px = 32;
    collision_box.y_offset_px = 0;
    collision_box.collision_width_px = 16;
    collision_box.collision_height_px = 16;
  } else if (state == State::BackDodgeShot) {
    collision_box.x_offset_px = 20;
    collision_box.y_offset_px = 0;
    collision_box.collision_width_px = 30;
    collision_box.collision_height_px = 16;
  } else {
    collision_box.x_offset_px = 30;
    collision_box.y_offset_px = 0;
    collision_box.collision_width_px = 18;
    collision_box.collision_height_px = 48;
  }
}

}  // namespace

void UpdatePlayerState(const ParameterServer& parameter_server,
                       const std::vector<AnimationEvent>& animation_events,
                       const PhysicsSystem& physics_system,
                       Registry& registry) {
  for (const auto id : registry.GetView<PlayerComponent>()) {
    UpdatePlayerState(id, parameter_server, animation_events, physics_system, registry);
  }
}

void UpdateComponentsFromState(const ParameterServer& parameter_server, Registry& registry) {
  UpdateMaxVelocity(parameter_server, registry);
}

void UpdatePlayerComponentsFromState(const ParameterServer& parameter_server,
                                     const std::vector<AnimationEvent>& animation_events,
                                     Registry& registry) {
  for (const auto id : registry.GetView<PlayerComponent>()) {
    UpdatePlayerComponentsFromState(id, parameter_server, animation_events, registry);
  }
}

}  // namespace platformer