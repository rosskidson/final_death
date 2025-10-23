#pragma once

#include <map>
#include <memory>

#include "animation/animation_event.h"
#include "animation/animated_sprite.h"
#include "common_types/actor_state.h"
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

class AnimationManager {
 public:
  AnimationManager(std::shared_ptr<Registry> registry) : registry_{std::move(registry)} {}

  void AddAnimation(AnimatedSprite sprite, Actor actor, State state){
    animated_sprites_.try_emplace(SpriteKey{actor, state}, std::move(sprite));
  }

  [[nodiscard]] AnimatedSprite& GetAnimation(Actor actor, State state) {
    return animated_sprites_.at(SpriteKey{actor, state});
  }

  [[nodiscard]] const AnimatedSprite& GetAnimation(Actor actor, State state) const {
    return animated_sprites_.at(SpriteKey{actor, state});
  }

  [[nodiscard]] std::vector<AnimationEvent> GetAnimationEvents() const;

  // void SwapAnimation(PlayerState action);

  // void Update(EntityId id);

  [[nodiscard]] const olc::Sprite* GetSprite(EntityId id) const;

 private:
  std::map<SpriteKey, AnimatedSprite> animated_sprites_;
  std::shared_ptr<Registry> registry_;
};
}  // namespace platformer