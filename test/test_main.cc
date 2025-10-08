#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <unordered_map>

#include "components.h"
#include "registry.h"

int add(int a, int b) { return a + b; }

TEST_CASE("all maps contain key") {
  std::unordered_set<platformer::EntityId> set_a{0, 1, 2, 3, 5, 6};
  std::unordered_set<platformer::EntityId> set_b{1, 4, 6};
  std::unordered_set<platformer::EntityId> set_c{1, 2, 4, 7};

  CHECK(platformer::internal::AllMapsContainKey(1, set_a, set_b, set_c));
  CHECK_FALSE(platformer::internal::AllMapsContainKey(0, set_a, set_b, set_c));
  CHECK_FALSE(platformer::internal::AllMapsContainKey(2, set_a, set_b, set_c));
  CHECK_FALSE(platformer::internal::AllMapsContainKey(4, set_a, set_b, set_c));
  CHECK_FALSE(platformer::internal::AllMapsContainKey(9, set_a, set_b, set_c));
}

TEST_CASE("GetIntersection") {
  std::unordered_map<platformer::EntityId, int> map_a{{0, 0}, {2, 0}, {4, 0}, {6, 0}, {8, 0}};
  std::unordered_map<platformer::EntityId, int> map_b{{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}};
  std::unordered_map<platformer::EntityId, int> map_c{{{0, 0}, {3, 0}, {4, 0}, {8, 0}}};

  const auto intersection = platformer::internal::GetIntersection(map_a, map_b, map_c);
  REQUIRE_EQ(intersection.size(), 2);
  CHECK_EQ(intersection[0], 0);
  CHECK_EQ(intersection[1], 4);
}

TEST_CASE("GetView") {
  using platformer::Acceleration;
  using platformer::Position;
  using platformer::Velocity;
  platformer::Registry r{};
  r.GetMap<Acceleration>()[0] = {};
  r.GetMap<Acceleration>()[1] = {};
  r.GetMap<Acceleration>()[3] = {};
  r.GetMap<Velocity>()[0] = {};
  r.GetMap<Velocity>()[3] = {};
  r.GetMap<Position>()[0] = {};
  r.GetMap<Position>()[1] = {};
  r.GetMap<Position>()[2] = {};
  r.GetMap<Position>()[3] = {};
  r.GetMap<Position>()[4] = {};

  const auto indices = r.GetView<Position, Velocity, Acceleration>();

  REQUIRE_EQ(indices.size(), 2);
  CHECK_EQ(indices[0], 0);
  CHECK_EQ(indices[1], 3);
}

TEST_CASE("HasComponent") {
  using platformer::Acceleration;
  using platformer::Position;
  using platformer::Velocity;
  platformer::Registry r{};
  r.GetMap<Acceleration>()[0] = {};
  CHECK(r.HasComponent<Acceleration>(0));
  CHECK_FALSE(r.HasComponent<Acceleration>(2));
}