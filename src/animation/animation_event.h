#pragma once

#include "common_types/entity.h"
#include "common_types/actor_state.h"

namespace platformer {

struct AnimationEvent {
  EntityId entity_id;
  std::string animation_key;
  std::string event_name;
};

}