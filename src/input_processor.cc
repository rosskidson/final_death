
#include "input_processor.h"

#include "basic_types.h"
#include "input_capture.h"
#include "player.h"
#include "sound.h"
#include "utils/developer_console.h"
#include "utils/game_clock.h"
#include "utils/parameter_server.h"

namespace platformer {

bool IsPlayerShooting(const Player& player) {
  return player.state == PlayerState::Shoot;
}

bool IsPlayerShootingOnGround(const Player& player) {
  return player.collisions.bottom && player.state == PlayerState::Shoot;
}

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

  if(input_.GetKey(InputAction::Left).held || input_.GetKey(InputAction::Right).held) {
    player.requested_states.insert(PlayerState::Walk);
  }

  if (input_.GetKey(InputAction::Left).held && !IsPlayerShootingOnGround(player)) {
    player.acceleration.x = -acceleration;
  } else if (input_.GetKey(InputAction::Right).held && !IsPlayerShootingOnGround(player)) {
    player.acceleration.x = +acceleration;
  } else {
    player.acceleration.x = 0;
  }

  if (input_.GetKey(InputAction::Crouch).held) {
    player.requested_states.insert(PlayerState::Crouch);
  }

  if (input_.GetKey(InputAction::Roll).pressed) {
    player.requested_states.insert(PlayerState::PreRoll);
  }

  if (input_.GetKey(InputAction::Jump).pressed) {
    player.requested_states.insert(PlayerState::PreJump);
  }

  if (input_.GetKey(InputAction::Shoot).held) {
    player.requested_states.insert(PlayerState::Shoot);
    if (!IsPlayerShooting(player)) {
      if (player.collisions.bottom) {
        player.velocity.x = 0;
        player.acceleration.x = 0;
      }
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

}  // namespace platformer