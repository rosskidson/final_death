#include "common_types/entity.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <unordered_map>

#include "common_types/components.h"
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

TEST_CASE("GetIntersectionOneMap") {
  std::unordered_map<platformer::EntityId, int> map_a{{0, 0}, {2, 0}, {4, 0}, {6, 0}, {8, 0}};

  const auto intersection = platformer::internal::GetIntersection(map_a);
  REQUIRE_EQ(intersection.size(), 5);
  CHECK_EQ(intersection[0], 0);
  CHECK_EQ(intersection[1], 2);
  CHECK_EQ(intersection[2], 4);
  CHECK_EQ(intersection[3], 6);
  CHECK_EQ(intersection[4], 8);
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

TEST_CASE("AddComponents") {
  using platformer::Acceleration;
  using platformer::Position;
  using platformer::Velocity;
  platformer::Registry r{};
  auto entity_id = r.AddComponents(Position{1., 2.}, Velocity{10., 0.}, Acceleration{0.5, 0.7});
  REQUIRE(r.GetMap<Position>().count(entity_id));
  CHECK_EQ(r.GetMap<Position>()[entity_id].x, 1.);
  CHECK_EQ(r.GetMap<Position>()[entity_id].y, 2.);
  CHECK_EQ(r.GetMap<Velocity>()[entity_id].x, 10.);
  CHECK_EQ(r.GetMap<Velocity>()[entity_id].y, 0.);
  CHECK_EQ(r.GetMap<Acceleration>()[entity_id].x, 0.5);
  CHECK_EQ(r.GetMap<Acceleration>()[entity_id].y, 0.7);
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

TEST_CASE("GetComponent") {
  using platformer::Acceleration;
  using platformer::Position;
  using platformer::Velocity;
  platformer::Registry r{};
  auto id_1 = r.AddComponents(Position{1., 2.}, Velocity{10., 0.}, Acceleration{0.5, 0.7});

  auto [pos, vel, acc] = r.GetComponents<Position, Velocity, Acceleration>(id_1);
  CHECK_EQ(pos.x, 1.);
  CHECK_EQ(pos.y, 2.);
  CHECK_EQ(vel.x, 10.);
  CHECK_EQ(vel.y, 0.);
  CHECK_EQ(acc.x, 0.5);
  CHECK_EQ(acc.y, 0.7);

  r.GetMap<Acceleration>()[id_1].x = 0.2;

  CHECK_EQ(acc.x, 0.2);
}

TEST_CASE("RemoveComponent") {
  using platformer::Acceleration;
  using platformer::Position;
  using platformer::Velocity;
  platformer::Registry r{};
  auto id_1 = r.AddComponents(Position{1., 2.}, Velocity{10., 0.}, Acceleration{0.5, 0.7});
  auto id_2 = r.AddComponents(Position{0., 1.}, Velocity{20., 0.});

  r.RemoveComponent(id_1);
  REQUIRE(r.GetMap<Position>().count(id_2));
  REQUIRE(r.GetMap<Velocity>().count(id_2));
  CHECK_EQ(r.GetMap<Position>()[id_2].x, 0.);
  CHECK_EQ(r.GetMap<Position>()[id_2].y, 1.);
  CHECK_EQ(r.GetMap<Velocity>()[id_2].x, 20.);
  CHECK_EQ(r.GetMap<Velocity>()[id_2].y, 0.);
}

TEST_CASE("player component") {
  using platformer::PlayerComponent;
  platformer::Registry r{};
  auto id_1 = r.AddComponents(PlayerComponent{});
  REQUIRE(r.GetMap<PlayerComponent>().count(id_1));
}

TEST_CASE("CombineViews") {
  std::vector<platformer::EntityId> vec1{0, 1, 2, 3};
  std::vector<platformer::EntityId> vec2{3, 4, 5, 6};
  std::vector<platformer::EntityId> vec3{6, 7, 8};
  const auto combined = platformer::CombineViews(vec1, vec2, vec3);

  REQUIRE_EQ(combined.size(), 9);
  auto itr = combined.begin();
  for (int i = 0; i < 9; ++i, ++itr) {
    CHECK_EQ(*itr, i);
  }
}