#include "projectile_system.h"

#include "common_types/components.h"

namespace platformer {

constexpr double kShotgunProjectileVelocity = 30.0;
constexpr double kShotgunNumPellets = 25.0;  // TODO(BT-01): parameter server type support
constexpr double kRifleProjectileVelocity = 30.0;

ProjectileSystem::ProjectileSystem(std::shared_ptr<ParameterServer> parameter_server,
                                   std::shared_ptr<const SpriteManager> animation_manager,
                                   std::shared_ptr<const RandomNumberGenerator> rng,
                                   std::shared_ptr<Registry> registry,
                                   int tile_size)
    : parameter_server_{std::move(parameter_server)},
      animation_manager_{std::move(animation_manager)},
      rng_{std::move(rng)},
      registry_{std::move(registry)},
      tile_size_{tile_size} {
  parameter_server_->AddParameter("projectiles/shotgun.vel", kShotgunProjectileVelocity,
                                  "How fast the bullets go. Unit: tile/s");

  parameter_server_->AddParameter("projectiles/num_shotgun_pellets", kShotgunNumPellets,
                                  "How many pellets in a shotgun blast");

  parameter_server_->AddParameter("projectiles/rifle.vel", kRifleProjectileVelocity,
                                  "How fast the bullet goes. Unit: tile/s");
}

Vector2d ProjectileSystem::GetBulletSpawnLocation(const EntityId entity_id) const {
  auto [facing, position] = registry_->GetComponentsConst<FacingDirection, Position>(entity_id);
  const auto spawn_location = animation_manager_->GetInsideSpriteLocation(entity_id);
  RB_CHECK(spawn_location.has_value());

  const int sprite_width = animation_manager_->GetSprite(entity_id).sprite_ptr->width;
  const int x_from_center = spawn_location->x_px - sprite_width / 2;

  const int sign = facing.facing == Direction::LEFT ? -1 : 1;
  const double tile_size_f = tile_size_;
  const double x_location = position.x + (sprite_width / 2 + sign * x_from_center) / tile_size_f;
  const double y_location = position.y + (spawn_location->y_px) / tile_size_f;
  return {x_location, y_location};
}

Velocity ProjectileSystem::GetShotgunPelletVelocity(const State state,
                                                    const Direction facing_direction) const {
  auto projectile_velocity = parameter_server_->GetParameter<double>("projectiles/shotgun.vel");
  constexpr double kSpreadWidth = 8;
  constexpr double kSpreadDepth = 10;
  constexpr double kHalfSpreadWidth = kSpreadWidth / 2;
  constexpr double kHalfSpreadDepth = kSpreadDepth / 2;
  if (state == State::UpShot || state == State::InAirDownShot) {
    Velocity vel{rng_->RandomFloat(-kHalfSpreadWidth, kHalfSpreadWidth),
                 projectile_velocity + rng_->RandomFloat(-kHalfSpreadDepth, kHalfSpreadDepth)};
    vel.y = state == State::InAirDownShot ? vel.y *= -1 : vel.y;
    return vel;
  }
  Velocity vel{projectile_velocity + rng_->RandomFloat(-kHalfSpreadDepth, kHalfSpreadDepth),
               rng_->RandomFloat(-kHalfSpreadWidth, kHalfSpreadWidth)};
  vel.x = facing_direction == Direction::LEFT ? vel.x *= -1 : vel.x;
  vel.x = state == State::BackShot ? vel.x *= -1 : vel.x;
  return vel;
}

Velocity ProjectileSystem::GetRifleBulletVelocity(const State state,
                                                  const Direction facing_direction) const {
  const auto bullet_vel = parameter_server_->GetParameter<double>("projectiles/rifle.vel");
  if (state == State::UpShot || state == State::InAirDownShot) {
    Velocity vel{0, bullet_vel};
    vel.y = state == State::InAirDownShot ? vel.y *= -1 : vel.y;
    return vel;
  }
  Velocity vel{bullet_vel, 0};
  vel.x = facing_direction == Direction::LEFT ? vel.x *= -1 : vel.x;
  vel.x = state == State::BackShot ? vel.x *= -1 : vel.x;
  return vel;
}

void ProjectileSystem::SpawnShotgunProjectiles(const EntityId entity_id) {
  const auto& state = registry_->GetComponent<StateComponent>(entity_id).state.GetState();
  const auto& facing_direction = registry_->GetComponent<FacingDirection>(entity_id).facing;
  // TODO(BT-01): Parameter server more type support
  const int num_pellets =
      static_cast<int>(parameter_server_->GetParameter<double>("projectiles/num_shotgun_pellets"));
  for (int i = 0; i < num_pellets; ++i) {
    const auto pos = GetBulletSpawnLocation(entity_id);

    DrawFunction draw_function{};
    draw_function.draw_fn = [](int px, int py, olc::PixelGameEngine* engine_ptr) {
      engine_ptr->Draw(px, py, olc::WHITE);
      engine_ptr->Draw(px + 1, py, olc::WHITE);
      engine_ptr->Draw(px, py + 1, olc::WHITE);
      engine_ptr->Draw(px - 1, py, olc::WHITE);
      engine_ptr->Draw(px, py - 1, olc::WHITE);
    };

    registry_->AddComponents(Position{pos.x, pos.y},
                             GetShotgunPelletVelocity(state, facing_direction),
                             SpriteComponent{"pellet"}, draw_function, Projectile{});
  }
}

void ProjectileSystem::SpawnRifleProjectile(const EntityId entity_id) {
  const auto& state = registry_->GetComponent<StateComponent>(entity_id).state.GetState();
  const auto& facing_direction = registry_->GetComponent<FacingDirection>(entity_id).facing;
  const auto pos = GetBulletSpawnLocation(entity_id);
  const auto vel = GetRifleBulletVelocity(state, facing_direction);

  std::string key{"bullet_01"};
  FacingDirection facing{};
  facing.facing = vel.x < 0 ? Direction::LEFT : Direction::RIGHT;

  if (std::abs(vel.y) > 1e-3) {
    key = "bullet_v_01";
    facing.facing = vel.y > 0 ? Direction::UP : Direction::DOWN;
  }

  registry_->AddComponents(Position{pos.x, pos.y}, vel,
                           AnimatedSpriteComponent{GameClock::NowGlobal(), {}, key}, facing,
                           Projectile{});
}

void ProjectileSystem::SpawnProjectiles(const std::vector<AnimationEvent>& animation_events) {
  for (const auto& event : animation_events) {
    if (event.event_name == "PlayerShoot") {
      const auto& weapon = registry_->GetComponent<PlayerComponent>(event.entity_id).weapon;
      if (weapon == Weapon::Rifle) {
        SpawnRifleProjectile(event.entity_id);
      } else if (weapon == Weapon::Shotgun) {
        SpawnShotgunProjectiles(event.entity_id);
      }
    }
  }
}

}  // namespace platformer