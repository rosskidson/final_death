#pragma once

#include <string>

#include "common_types/entity.h"

namespace platformer {

struct AnimationEvent {
  EntityId entity_id;
  std::string animation_key;
  std::string event_name;
};

}  // namespace platformer