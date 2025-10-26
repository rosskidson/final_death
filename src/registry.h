#pragma once

#include <tuple>
#include <unordered_map>

#include "common_types/components.h"
#include "registry_helpers.h"
#include "utils/check.h"

namespace platformer {

// Implements a registry class for the Entity-Component-System software patttern.
class Registry {
 public:
  Registry() = default;

  // Usage:
  // for(auto id : registry.GetView<Postion, Velocity, Acceleration>()) {
  //     ...
  // }
  template <typename... Args>
  std::vector<EntityId> GetView() const {
    return internal::GetIntersection(internal::GetByType<Args>(maps_tuple_)...);
  }

  template <typename Component>
  std::unordered_map<EntityId, Component>& GetMap() {
    return internal::GetByType<Component>(maps_tuple_);
  }

  template <typename Component>
  const std::unordered_map<EntityId, Component>& GetMap() const {
    return internal::GetByType<Component>(maps_tuple_);
  }

  template <typename Component>
  bool HasComponent(EntityId id) const {
    const auto& map = GetMap<Component>();
    return map.count(id) != 0;
  }

  // Usage:
  // auto id = registry.AddComponent(Position{1., 2.},
  //                                 Velocity{10., 0.},
  //                                 Acceleration{0.5, 0.7});
  template <typename... Args>
  EntityId AddComponents(Args&&... args) {
    EntityId id = next_id_++;
    ((internal::GetByType<std::decay_t<Args>>(maps_tuple_)[id] = std::forward<Args>(args)), ...);
    return id;
  }

  // Usage:
  // auto [pos, vel, acc] = registry.GetComponents<Position, Velocity, Acceleration>(id);
  // These are still references even though the auto is without an ampersand.
  // Note: There is no safety on the types you pass in.
  //       If the component doesn't exist, it will be default initialized.
  //
  // TODO:: Add saftey
  // TODO:: Add const version
  template <typename... Components>
  auto GetComponents(EntityId id) {
    return std::tie(GetMap<Components>()[id]...);
  }

  // As above, but just one component.
  template <typename T>
  auto& GetComponent(EntityId id) {
    auto& map = GetMap<T>();
    RB_CHECK(map.count(id));
    return map[id];
  }

  template <typename T>
  const auto& GetComponent(EntityId id) const {
    auto& map = GetMap<T>();
    RB_CHECK(map.count(id));
    return map.at(id);
  }

  // Removes the id from all component maps.
  void RemoveComponent(EntityId id) {
    std::apply([&](auto&&... args) { RemoveComponentImpl(id, args...); }, maps_tuple_);
  }

 private:
  template <typename... Args>
  void RemoveComponentImpl(EntityId id, Args&&... args) {
    ((args.erase(id)), ...);
  }

  EntityId next_id_{};
  std::tuple<std::unordered_map<EntityId, Position>,
             std::unordered_map<EntityId, Velocity>,
             std::unordered_map<EntityId, Acceleration>,
             std::unordered_map<EntityId, CollisionBox>,
             std::unordered_map<EntityId, Collision>,
             std::unordered_map<EntityId, FacingDirection>,
             std::unordered_map<EntityId, StateComponent>,
             std::unordered_map<EntityId, PlayerComponent>,
             std::unordered_map<EntityId, DistanceFallen>,
             std::unordered_map<EntityId, Projectile>>
      maps_tuple_;
};

}  // namespace platformer
