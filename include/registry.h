#pragma once

#include <unordered_map>

#include "components.h"
#include "registry_helpers.h"

namespace platformer {

// Implements a registry class for the Entity-Component-System software patttern.
class Registry {
 public:
  Registry() = default;

  template <typename T, typename... Args>
  std::vector<EntityId> GetView() const {
    return std::apply(
        [&](const auto&... maps) {
          return internal::GetIntersection(internal::GetByType<T>(maps_tuple_),
                                           internal::GetByType<Args>(maps_tuple_)...);
        },
        maps_tuple_);
  }

  template <typename T>
  std::unordered_map<EntityId, T>& GetMap() {
    return internal::GetByType<T>(maps_tuple_);
  }

  template <typename T>
  const std::unordered_map<EntityId, T>& GetMap() const {
    return internal::GetByType<T>(maps_tuple_);
  }

  template <typename T>
  bool HasComponent(EntityId id) const {
    const auto& map = GetMap<T>();
    return map.count(id) != 0;
  }

 private:
  uint64_t next_id_{};
  std::tuple<std::unordered_map<EntityId, Position>,
             std::unordered_map<EntityId, Velocity>,
             std::unordered_map<EntityId, Acceleration>,
             std::unordered_map<EntityId, CollisionBox>,
             std::unordered_map<EntityId, Collision>>
      maps_tuple_;
};

}  // namespace platformer
