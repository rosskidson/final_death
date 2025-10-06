#include <unordered_map>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "registry.h"

int add(int a, int b) { return a + b; }

TEST_CASE("Get all indices") {
  platformer::Registry r{};
  r.Accelerations()[0] = {};
  r.Accelerations()[3] = {};
  r.Velocities()[0] = {};
  r.Positions()[3] = {};
  r.Positions()[2] = {};
  r.Positions()[1] = {};
  r.Positions()[1] = {};

  const auto all_indices =
      platformer::internal::GetIndices(r.Positions(), r.Velocities(), r.Accelerations());

  REQUIRE_EQ(all_indices.size(), 3);
  REQUIRE_EQ(all_indices[0].size(), 2);
  CHECK(all_indices[0].count(0));
  CHECK(all_indices[0].count(3));
  REQUIRE_EQ(all_indices[1].size(), 1);
  CHECK(all_indices[0].count(0));
  REQUIRE_EQ(all_indices[2].size(), 3);
  CHECK(all_indices[2].count(1));
  CHECK(all_indices[2].count(2));
  CHECK(all_indices[2].count(3));
}

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