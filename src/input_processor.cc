

#include "input_processor.h"

#include <memory>

#include "basic_types.h"
#include "components.h"
#include "input_capture.h"
#include "actor_state.h"
#include "registry_helpers.h"
#include "sound.h"
#include "utils/check.h"
#include "utils/developer_console.h"
#include "utils/game_clock.h"
#include "utils/logging.h"
#include "utils/parameter_server.h"

namespace platformer {

constexpr double kAcceleration = 50.0;
constexpr double kJumpVel = 21.0;

InputProcessor::InputProcessor(std::shared_ptr<ParameterServer> parameter_server,
                               std::shared_ptr<const SoundPlayer> sound_player,
                               std::shared_ptr<Registry> registry,
                               olc::PixelGameEngine* engine_ptr)
    : input_{engine_ptr},
      parameter_server_{std::move(parameter_server)},
      sound_player_{std::move(sound_player)},
      registry_{std::move(registry)},
      engine_ptr_{engine_ptr} {
  parameter_server_->AddParameter("physics/player.acceleration", kAcceleration,
                                  "Horizontal acceleration of the player, unit: tile/s²");
  parameter_server_->AddParameter(
      "physics/jump.velocity", kJumpVel,
      "The instantaneous vertical velocity when you jump, unit: tile/s");
}

bool InputProcessor::ProcessInputs(EntityId player_id) {
  if (engine_ptr_->IsConsoleShowing()) {
    return true;
  }

  RB_CHECK(registry_->HasComponent<Acceleration>(player_id));
  RB_CHECK(registry_->HasComponent<PlayerComponent>(player_id));
  auto [acceleration, state] = registry_->GetComponents<Acceleration, PlayerComponent>(player_id);

  input_.Capture();

  const auto walking_acceleration =
      parameter_server_->GetParameter<double>("physics/player.acceleration");
  const auto jump_velocity = parameter_server_->GetParameter<double>("physics/jump.velocity");

  if (input_.GetKey(InputAction::Left).held || input_.GetKey(InputAction::Right).held) {
    state.requested_states.insert(State::Walk);
  }

  if (input_.GetKey(InputAction::Left).held) {
    acceleration.x = -walking_acceleration;
  } else if (input_.GetKey(InputAction::Right).held) {
    acceleration.x = +walking_acceleration;
  } else {
    acceleration.x = 0;
  }

  if (input_.GetKey(InputAction::Up).held) {
    state.requested_states.insert(State::AimUp);
  }

  if (input_.GetKey(InputAction::Down).held) {
    state.requested_states.insert(State::Crouch);
  }

  if (input_.GetKey(InputAction::Roll).pressed) {
    state.requested_states.insert(State::PreRoll);
  }

  if (input_.GetKey(InputAction::Jump).pressed) {
    state.requested_states.insert(State::PreJump);
  }

  if (input_.GetKey(InputAction::Shoot).held) {
    state.requested_states.insert(State::Shoot);
  }

  if (input_.GetKey(InputAction::Backshot).held) {
    state.requested_states.insert(State::BackShot);
  }

  if (input_.GetKey(InputAction::Suicide).pressed) {
    state.requested_states.insert(State::PreSuicide);
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

  if (input_.GetKey(InputAction::Quit).released) {
    return false;
  }

  return true;
}

}  // namespace platformer