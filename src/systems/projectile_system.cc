#include "projectile_system.h"

namespace platformer {

constexpr double kShotgunProjectileVelocity = 30.0;
constexpr double kShotgunNumPellets = 25.0;  //TODO(UL-01): parameter server type support

ProjectileSystem::ProjectileSystem(std::shared_ptr<ParameterServer> parameter_server,
                     std::shared_ptr<const AnimationManager> animation_manager,
                     std::shared_ptr<const RandomNumberGenerator> rng,
                     std::shared_ptr<Registry> registry,
                     int tile_size):
    parameter_server_{std::move(parameter_server)},
    animation_manager_{std::move(animation_manager)},
    rng_{std::move(rng)},
    registry_{std::move(registry)},
    tile_size_{tile_size} {
  parameter_server_->AddParameter("projectiles/shotgun.vel", kShotgunProjectileVelocity,
                                   "How fast the bullets go. Unit: tile/s");

  parameter_server_->AddParameter("projectiles/num_shotgun_pellets", kShotgunNumPellets,
                                   "How many pellets in a shotgun blast");

}

Vector2d ProjectileSystem::GetBulletSpawnLocation(const EntityId entity_id) const {
  auto [facing, position, state] =
      registry_->GetComponentsConst<FacingDirection, Position, StateComponent>(entity_id);
  const auto spawn_location = animation_manager_->GetInsideSpriteLocation(entity_id);
  RB_CHECK(spawn_location.has_value());

  const int sprite_width = animation_manager_->GetSprite(entity_id)->width;
  const int x_from_center = spawn_location->x_px - sprite_width / 2;

  const int sign = facing.facing == Direction::LEFT ? -1 : 1;
  const double tile_size_f = tile_size_;
  const double x_location = position.x + (sprite_width / 2 + sign * x_from_center) / tile_size_f;
  const double y_location = position.y + (spawn_location->y_px) / tile_size_f;
  return {x_location, y_location};
}

Velocity ProjectileSystem::GetShotgunPelletVelocity(const State state,
                                  const Direction facing_direction) const {
  auto projectile_velocity =
      parameter_server_->GetParameter<double>("projectiles/shotgun.vel");
  constexpr double kSpreadWidth = 5;
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

void ProjectileSystem::SpawnShotgunProjectiles(const EntityId entity_id) {
  const auto& state = registry_->GetComponent<StateComponent>(entity_id).state.GetState();
  const auto& facing_direction = registry_->GetComponent<FacingDirection>(entity_id).facing;
  // TODO(UL-01): Parameter server more type support
  const int num_pellets = static_cast<int>(parameter_server_->GetParameter<double>("projectiles/num_shotgun_pellets"));
  for (int i = 0; i < num_pellets; ++i) {
    const auto pos = GetBulletSpawnLocation(entity_id);
    registry_->AddComponents(Position{pos.x, pos.y},
                           GetShotgunPelletVelocity(state, facing_direction),
                           Projectile{});
  }

  const auto pos = GetBulletSpawnLocation(entity_id);

  registry_->AddComponents(Position{pos.x, pos.y},
                         GetShotgunPelletVelocity(state, facing_direction),
                         Projectile{});//, Animation{GameClock::NowGlobal(), "bullet_01"});
}

void ProjectileSystem::SpawnRifleProjectile(const EntityId entity_id) {}

void ProjectileSystem::SpawnProjectiles(const std::vector<AnimationEvent>& animation_events) {
  for (const auto& event : animation_events) {
    if (event.event_name == "ShootShotgun") {
      SpawnShotgunProjectiles(event.entity_id);
    } else if(event.event_name == "ShootRifle") {
      SpawnRifleProjectile(event.entity_id);
    }
  }
}


}