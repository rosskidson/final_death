#pragma once

#include "registry.h"
#include "registry_helpers.h"
#include "utils/parameter_server.h"

namespace platformer {

// Sets the player state based on current state and requested states.
void UpdateState(const ParameterServer& parameter_server, EntityId player_id, Registry& registry);


  // Applies rules to the player based on the current state.
  // E.g. disallow movement when shooting.
  void UpdateComponentsFromState(const ParameterServer& parameter_server, Registry& registry);

}  // namespace platformer