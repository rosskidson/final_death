#pragma once

#include <map>
#include <memory>

#include "animation/animated_sprite.h"
#include "animation/animation_event.h"
#include "common_types/actor_state.h"
#include "common_types/entity.h"
#include "registry.h"
#include "registry_helpers.h"

namespace platformer {

// Needed for specific actions, e.g. spawning bullets
struct InsideSpriteLocation {
  int x_px{};  // Measured from the left
  int y_px{};  // Measured from the bottom
};

class AnimationManager {
 public:
  AnimationManager(std::shared_ptr<Registry> registry) : registry_{std::move(registry)} {}

  void AddAnimation(const std::string& key, AnimatedSprite sprite);

  AnimatedSprite& GetAnimation(const std::string& key);
  const AnimatedSprite& GetAnimation(const std::string& key) const;

  void AddInsideSpriteLocation(const std::string& key, InsideSpriteLocation location);

  [[nodiscard]] std::optional<InsideSpriteLocation> GetInsideSpriteLocation(EntityId id) const;

  [[nodiscard]] std::vector<AnimationEvent> GetAnimationEvents();

  [[nodiscard]] Sprite GetSprite(EntityId id) const;

 private:
  std::map<std::string, AnimatedSprite> animated_sprites_;
  std::map<std::string, InsideSpriteLocation> inside_sprite_locations_;
  std::shared_ptr<Registry> registry_;
};
}  // namespace platformer