#pragma once

#include <map>
#include <memory>

#include "animated_sprite.h"
#include "state.h"
#include "registry.h"
#include "registry_helpers.h"

namespace platformer {

class SpriteKey {
 public:
  SpriteKey() = default;
  SpriteKey(Actor actor, uint8_t state) : actor_{actor}, state_{state} {}

  template <typename StateEnum>
  SpriteKey(Actor actor, StateEnum state) : actor_{actor}, state_{static_cast<uint8_t>(state)} {}

  bool operator<(const SpriteKey& other) const {
    if (actor_ != other.actor_) {
      return actor_ < other.actor_;
    }
    return state_ < other.state_;
  }

 private:
  Actor actor_;
  uint8_t state_;
};

class AnimationManager {
 public:
  AnimationManager(std::shared_ptr<Registry> registry) : registry_{std::move(registry)} {}

  template <typename StateEnum>
  void AddAnimation(AnimatedSprite sprite, Actor actor, StateEnum state){
    animated_sprites_.try_emplace(SpriteKey{actor, state}, std::move(sprite));
  }

  // void SwapAnimation(PlayerState action);

  // void Update(EntityId id);

  [[nodiscard]] const olc::Sprite* GetSprite(EntityId id) const;

 private:
  std::map<SpriteKey, AnimatedSprite> animated_sprites_;
  std::shared_ptr<Registry> registry_;
};
}  // namespace platformer