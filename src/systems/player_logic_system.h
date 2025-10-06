#pragma once

#include "animation/animation_event.h"
#include "registry.h"
#include "systems/physics_system.h"
#include "utils/parameter_server.h"

namespace platformer {

// Sets the player state based on current state and requested states.
void UpdatePlayerState(const ParameterServer& parameter_server,
                       const std::vector<AnimationEvent>& animation_events,
                       const PhysicsSystem& physics_system,
                       Registry& registry);

// Applies rules to the state based on the current state.
// E.g. disallow movement when shooting.
void UpdateComponentsFromState(const ParameterServer& parameter_server, Registry& registry);

// As above, but rules that only apply to PlayerComponent
void UpdatePlayerComponentsFromState(const ParameterServer& parameter_server,
                                     const std::vector<AnimationEvent>& animation_events,
                                     Registry& registry);

}  // namespace platformer