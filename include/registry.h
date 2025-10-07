#pragma once

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include "components.h"

namespace platformer {

using EntityId = uint64_t;

namespace internal {

template <typename T>
std::set<EntityId> GetIndices(const std::unordered_map<EntityId, T>& map) {
  std::set<EntityId> ids;
  for (const auto& [id, _] : map) {
    ids.insert(id);
  }
  return ids;
}

template <typename T, typename... Args>
std::vector<std::set<EntityId>> GetIndices(const T& map, const Args&... args) {
  if constexpr (sizeof...(Args) > 0) {
    std::vector<std::set<EntityId>> all_indices{GetIndices(args...)};
    all_indices.push_back(GetIndices(map));
    return all_indices;
  }
  return std::vector<std::set<EntityId>>{GetIndices(map)};
}

template <typename A, typename B>
std::unordered_set<EntityId> GetIntersection(const std::unordered_map<EntityId, A>& map_a,
                                             const std::unordered_map<EntityId, B>& map_b) {
  std::unordered_set<EntityId> intersection{};
  const auto& outer_map = map_a.size() < map_b.size() ? map_a : map_b;
  const auto& inner_map = map_a.size() < map_b.size() ? map_b : map_a;
  for (const auto& [idx, _] : outer_map) {
    if (inner_map.count(idx) > 0) {
      intersection.insert(idx);
    }
  }
  return intersection;
}

template <typename T, typename... Args>
bool AllMapsContainKey(const EntityId id, const T& map) {
  return map.count(id) != 0;
}

template <typename T, typename... Args>
bool AllMapsContainKey(const EntityId id, const T& map, const Args&... args) {
  return AllMapsContainKey(id, map) && AllMapsContainKey(id, args...);
}

template <typename T, typename... Args>
std::vector<EntityId> GetIntersection(const T& map, const Args&... args) {
  std::vector<EntityId> intersection;
  for (const auto& [idx, _] : map) {
    if (AllMapsContainKey(idx, args...)) {
      intersection.push_back(idx);
    }
  }
  // Sort the results to keep the output deterministic.
  std::sort(intersection.begin(), intersection.end());
  return intersection;
}

}  // namespace internal

// Implements a registry class for the Entity-Component-System software patttern.
class Registry {
 public:
  Registry() = default;

  template <typename T, typename... Args>
  std::vector<EntityId> GetView() const;

  template <typename T>
  std::unordered_map<EntityId, T>& GetMap();

  std::unordered_map<EntityId, Position>& Positions() { return positions_; }
  std::unordered_map<EntityId, Velocity>& Velocities() { return velocities_; }
  std::unordered_map<EntityId, Acceleration>& Accelerations() { return accelerations_; }
  std::unordered_map<EntityId, CollisionBox>& CollisionBoxes() { return collision_boxes_; }
  std::unordered_map<EntityId, Collision>& Collisions() { return collisions_; }

 private:
  std::unordered_map<EntityId, Position> positions_;
  std::unordered_map<EntityId, Velocity> velocities_;
  std::unordered_map<EntityId, Acceleration> accelerations_;
  std::unordered_map<EntityId, CollisionBox> collision_boxes_;
  std::unordered_map<EntityId, Collision> collisions_;
};

// TODO:: Move to .cpp along with expilcit instanciation.

template <>
std::unordered_map<EntityId, Position>& Registry::GetMap<Position>();

template <>
std::unordered_map<EntityId, Velocity>& Registry::GetMap<Velocity>();


}  // namespace platformer
