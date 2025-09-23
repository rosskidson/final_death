#pragma once

#include "player.h"

namespace platformer {

// Sets the player state based on current state and requested states.
void UpdateState(Player& player);

// Applies rules to the player based on the current state. 
// E.g. disallow movement when shooting.
void UpdatePlayerFromState(Player& player);

}  // namespace platformer