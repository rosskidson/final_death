#include "player.h"
#include "player_state.h"

namespace platformer {

bool IsInterruptibleState(PlayerState state) {
  switch (state) {
    case PlayerState::Shoot:
    case PlayerState::InAirShoot:
    case PlayerState::CrouchShoot:
    case PlayerState::PreRoll:
    case PlayerState::PreJump:
    case PlayerState::Suicide:
      return true;
  }
  return false;
}

void SetPlayerState(Player& player, PlayerState new_state) {
  if (!player.animation_manager.GetActiveAnimation().Expired() && !IsInterruptibleState(player.state)) {
    return;
  }
  if(!IsInterruptibleState(new_state)) {
    player.state = new_state;
    return;
  }
  if(player.collisions.bottom) {
    player.state = PlayerState::InAir;
    return;
  }
  player.state = new_state;
}

}  // namespace platformer