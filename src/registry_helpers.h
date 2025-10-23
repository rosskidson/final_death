#pragma once

#include "common_types/entity.h"

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace platformer {

namespace internal {

template <typename T>
bool AllMapsContainKey(const EntityId id, const T& map) {
  return map.count(id) != 0;
}

template <typename T, typename... Args>
bool AllMapsContainKey(const EntityId id, const T& map, const Args&... args) {
  return map.count(id) != 0 && AllMapsContainKey(id, args...);
}

template <typename T, typename... Args>
std::vector<EntityId> GetIntersection(const T& map, const Args&... args) {
  std::vector<EntityId> intersection;
  for (const auto& [idx, _] : map) {
    if (((args.count(idx) != 0) && ...)) {
      intersection.push_back(idx);
    }
  }
  // Sort the results to keep the output deterministic.
  std::sort(intersection.begin(), intersection.end());
  return intersection;
}

template <typename Component, typename Tuple>
decltype(auto) GetByType(Tuple& t) {
  return std::get<std::unordered_map<EntityId, Component>>(t);
}

}  // namespace internal
}  // namespace platformer