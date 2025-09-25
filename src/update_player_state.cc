#include "update_player_state.h"

#include <chrono>

#include "physics_engine.h"
#include "player_state.h"
#include "utils/game_clock.h"
#include "utils/logging.h"
#include "utils/parameter_server.h"

namespace platformer {

#define TRY_SET_STATE(player, STATE)              \
  do {                                            \
    if ((player).requested_states.count(STATE)) { \
      (player).state = (STATE);                   \
      return;                                     \
    }                                             \
  } while (0)

namespace {

bool IsInterruptibleState(PlayerState state) {
  switch (state) {
    case PlayerState::Shoot:
    case PlayerState::InAirShoot:
    case PlayerState::InAirDownShoot:
    case PlayerState::CrouchShoot:
    case PlayerState::PreRoll:
    case PlayerState::Roll:
    case PlayerState::PreJump:
    case PlayerState::Landing:
    case PlayerState::Suicide:
      return false;
  }
  return true;
}

bool Shooting(const PlayerState state) {
  return state == PlayerState::Shoot || state == PlayerState::CrouchShoot ||
         state == PlayerState::InAirShoot;
}

bool Squish(const AxisCollisions& collisions) {
  return collisions.lower_collision && collisions.upper_collision;
}

void UpdateStateImpl(const ParameterServer& parameter_server,
                     const PhysicsEngine& physics,
                     Player& player) {
  if (!player.animation_manager.GetActiveAnimation().Expired() &&
      !IsInterruptibleState(player.state)) {
    // If the player is shooting in the air and lands, transition the animation to the standing
    // pose.  Only do this after the first few frame to avoid double fire.
    if ((player.state == PlayerState::InAirShoot || player.state == PlayerState::InAirDownShoot) &&
        player.collisions.bottom &&
        ((GameClock::NowGlobal() - player.animation_manager.GetActiveAnimation().GetStartTime())
                 .count() /
             1e6 >
         100)) {
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
      AxisCollisions collisions_x = physics.CheckPlayerAxisCollision(player, Axis::X);
      AxisCollisions collisions_y = physics.CheckPlayerAxisCollision(player, Axis::Y);
      if (!Squish(collisions_x) && !Squish(collisions_y)) {
        player.state = PlayerState::PostRoll;
      }
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
    player.roll_start_time = GameClock::NowGlobal();
    player.state = PlayerState::Roll;
    return;
  }

  // Remaining non-interruptable states
  TRY_SET_STATE(player, PlayerState::PreRoll);

  if (player.collisions.bottom && player.collisions.bottom_changed && player.hard_landing) {
    player.hard_landing = false;
    player.state = PlayerState::Landing;
    return;
  }

  // Latch post-roll
  if (player.state == PlayerState::PostRoll &&
      !player.animation_manager.GetActiveAnimation().Expired()) {
    return;
  }

  // InAir has priority over other states.
  if (!player.collisions.bottom) {
    if (player.requested_states.count(PlayerState::InAirDownShoot)) {
      player.state = PlayerState::InAirDownShoot;
    } else {
      player.state = PlayerState::InAir;
    }
    return;
  }

  // Lower priority interruptable states.
  TRY_SET_STATE(player, PlayerState::PreJump);
  TRY_SET_STATE(player, PlayerState::Crouch);
  TRY_SET_STATE(player, PlayerState::PreSuicide);

  // The landing crouch only has priority over walk/idle. Latched until expiration
  // if (player.collisions.bottom_changed ||
  //     (player.state == PlayerState::Landing &&
  //      !player.animation_manager.GetActiveAnimation().Expired())) {
  //   player.state = PlayerState::Landing;
  //   return;
  // }

  TRY_SET_STATE(player, PlayerState::Walk);

  player.state = PlayerState::Idle;
}

}  // namespace

void UpdatePlayerFromState(const ParameterServer& parameter_server, Player& player) {
  // Disallow movement during firing.
  if ((player.state == PlayerState::Shoot || player.state == PlayerState::CrouchShoot ||
       player.state == PlayerState::Landing) &&
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

  if (player.state == PlayerState::PreJump) {
    if (player.velocity.x != 0) {
      player.cached_velocity.x = player.velocity.x;
    }
    player.velocity.x = 0;
    player.acceleration.x = 0;
  }

  // Roll
  const auto roll_vel = parameter_server.GetParameter<double>("physics/roll.x.vel");
  if (player.state == PlayerState::Roll) {
    if (player.facing_left) {
      player.velocity.x = -roll_vel;
    } else {
      player.velocity.x = roll_vel;
    }
    if ((player.collisions.left && player.acceleration.x > 0) ||
        (player.collisions.right && player.acceleration.x < 0)) {
      player.velocity.x *= -1;
    }

    // TODO:: This is hacky and should be configured better.
    player.x_offset_px = 32;
    player.y_offset_px = 0;
    player.collision_width_px = 16;
    player.collision_height_px = 16;
  } else {
    player.x_offset_px = 30;
    player.y_offset_px = 0;
    player.collision_width_px = 18;
    player.collision_height_px = 48;
  }
}

void UpdateState(const ParameterServer& parameter_server,
                 const PhysicsEngine& physics,
                 Player& player) {
  UpdateStateImpl(parameter_server, physics, player);
  player.requested_states.clear();
}

}  // namespace platformer