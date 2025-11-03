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

struct SpriteKey {
  SpriteKey() = default;
  SpriteKey(Actor actor, State state) : actor{actor}, state{state} {}

  bool operator<(const SpriteKey& other) const {
    if (actor != other.actor) {
      return actor < other.actor;
    }
    return state < other.state;
  }

  Actor actor;
  State state;
};

// Needed for specific actions, e.g. spawning bullets
struct InsideSpriteLocation {
  int x_px{};  // Measured from the left
  int y_px{};  // Measured from the bottom
};

class AnimationManager {
 public:
  AnimationManager(std::shared_ptr<Registry> registry) : registry_{std::move(registry)} {}

  void AddAnimation(AnimatedSprite sprite, Actor actor, State state);

  [[nodiscard]] AnimatedSprite& GetAnimation(Actor actor, State state);
  [[nodiscard]] const AnimatedSprite& GetAnimation(Actor actor, State state) const;

  void AddInsideSpriteLocation(InsideSpriteLocation location, Actor actor, State state);

  [[nodiscard]] std::optional<InsideSpriteLocation> GetInsideSpriteLocation(EntityId id) const;

  [[nodiscard]] std::vector<AnimationEvent> GetAnimationEvents() const;

  [[nodiscard]] const olc::Sprite* GetSprite(EntityId id) const;

 private:
  std::map<SpriteKey, AnimatedSprite> animated_sprites_;
  std::map<SpriteKey, InsideSpriteLocation> inside_sprite_locations_;
  std::shared_ptr<Registry> registry_;
};
}  // namespace platformer