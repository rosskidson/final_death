#pragma once

#include <memory>

#include "animation/animation_manager.h"
#include "utils/parameter_server.h"
#include "utils/random_number_generator.h"
#include "registry.h"

namespace platformer {

class ProjectileSystem {
  public:
    ProjectileSystem(std::shared_ptr<ParameterServer> parameter_server,
                     std::shared_ptr<const AnimationManager> animation_manager,
                     std::shared_ptr<const RandomNumberGenerator> rng,
                     std::shared_ptr<Registry> registry,
                     int tile_size);

    void SpawnProjectiles(const std::vector<AnimationEvent>& animation_events);

  private:
    void SpawnShotgunProjectiles(const EntityId entity_id);
    void SpawnRifleProjectile(const EntityId entity_id);

    Vector2d GetBulletSpawnLocation(const EntityId entity_id) const;
    Velocity GetShotgunPelletVelocity(const State state,
                                      const Direction facing_direction) const;

    std::shared_ptr<ParameterServer> parameter_server_;
    std::shared_ptr<const AnimationManager> animation_manager_;
    std::shared_ptr<const RandomNumberGenerator> rng_;
    std::shared_ptr<Registry> registry_;
    int tile_size_;
};

}