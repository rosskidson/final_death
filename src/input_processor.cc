
#include "input_processor.h"

#include "basic_types.h"
#include "input_capture.h"
#include "sound.h"
#include "utils/developer_console.h"
#include "utils/game_clock.h"
#include "utils/parameter_server.h"

namespace platformer {

InputProcessor::InputProcessor(std::shared_ptr<const ParameterServer> parameter_server,
                               std::shared_ptr<const SoundPlayer> sound_player,
                               olc::PixelGameEngine* engine_ptr)
    : input_{engine_ptr},
      parameter_server_{std::move(parameter_server)},
      sound_player_{std::move(sound_player)},
      engine_ptr_{engine_ptr} {}

bool InputProcessor::ProcessInputs(Player& player) {
  if (engine_ptr_->IsConsoleShowing()) {
    return true;
  }

  input_.Capture();

  const auto acceleration = parameter_server_->GetParameter<double>("physics/player.acceleration");
  const auto jump_velocity = parameter_server_->GetParameter<double>("physics/jump.velocity");

  if (input_.GetKey(InputAction::Left).pressed || input_.GetKey(InputAction::Right).pressed) {
    player.animation_manager.StartAction(Action::Walk);
  }

  if (input_.GetKey(InputAction::Left).held && !IsPlayerShootingOnGround(player)) {
    player.acceleration.x = -acceleration;
  } else if (input_.GetKey(InputAction::Right).held && !IsPlayerShootingOnGround(player)) {
    player.acceleration.x = +acceleration;
  } else {
    if (!IsPlayerShooting(player)) {
      player.animation_manager.EndAction(Action::Walk);
    }
    player.acceleration.x = 0;
  }

  if (input_.GetKey(InputAction::Crouch).pressed) {
    player.animation_manager.StartAction(Action::Crouch);
  }
  if (input_.GetKey(InputAction::Crouch).released) {
    player.animation_manager.EndAction(Action::Crouch);
  }

  if (input_.GetKey(InputAction::Roll).pressed) {
    player.animation_manager.StartAction(Action::Roll);
  }

  if (input_.GetKey(InputAction::Jump).pressed) {
    if (player.collisions.bottom) {
      player.velocity.y = jump_velocity;
    }
  }
  if (input_.GetKey(InputAction::Shoot).held) {
    if (!IsPlayerShooting(player)) {
      if (player.collisions.bottom) {
        player.velocity.x = 0;
        player.acceleration.x = 0;
      }
      player.started_shooting = GameClock::NowGlobal();
      player.animation_manager.StartAction(Action::Shoot);
    }
  }

  if (input_.GetKey(InputAction::Quit).released) {
    return false;
  }

  if (input_.GetKey(InputAction::Console).pressed) {
    GameClock::PauseGlobal();
    engine_ptr_->ConsoleShow(olc::Key::TAB, false);
    engine_ptr_->ConsoleCaptureStdOut(true);
    PrintConsoleWelcome();
  }
  if (!engine_ptr_->IsConsoleShowing()) {
    GameClock::ResumeGlobal();
    // engine_ptr_->ConsoleCaptureStdOut(false);
  }

  return true;
}

bool InputProcessor::IsPlayerShooting(const Player& player) {
  const auto shoot_delay = parameter_server_->GetParameter<double>("timing/shoot.delay");
  return (GameClock::NowGlobal() - player.started_shooting).count() / 1e6 < shoot_delay;
}

bool InputProcessor::IsPlayerShootingOnGround(const Player& player) {
  if (!player.collisions.bottom) {
    return false;
  }
  const auto shoot_delay = parameter_server_->GetParameter<double>("timing/shoot.delay");
  return (GameClock::NowGlobal() - player.started_shooting).count() / 1e6 < shoot_delay;
}

}  // namespace platformer