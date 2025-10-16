#pragma once

#include <map>
#include <memory>

#include "animated_sprite.h"
#include "player_state.h"
#include "registry.h"
#include "registry_helpers.h"

namespace platformer {

enum class Actor : uint8_t { Protagonist, Enemy, BadassEnemy, Boss };

class SpriteKey {
 public:
  SpriteKey() = default;
  SpriteKey(Actor actor, PlayerState state) : actor_(actor), state_(state) {}

  bool operator<(const SpriteKey& other) const {
    if (actor_ != other.actor_) {
      return actor_ < other.actor_;
    }
    return state_ < other.state_;
  }

 private:
  Actor actor_;
  PlayerState state_;
};

class AnimationManager {
 public:
  AnimationManager(std::shared_ptr<Registry> registry) : registry_{std::move(registry)} {}

  void AddAnimation(AnimatedSprite sprite, Actor actor, PlayerState state);

  // [[nodiscard]] const AnimatedSprite& GetAnimation(PlayerState action) const;
  // [[nodiscard]] AnimatedSprite& GetAnimation(PlayerState action);

  // [[nodiscard]] const AnimatedSprite& GetActiveAnimation() const;
  // [[nodiscard]] AnimatedSprite& GetActiveAnimation();

  // void SwapAnimation(PlayerState action);

  void Update(EntityId id);

  // void StartAction(Action action);
  // void AddAction(Action action);
  // void EndAction(Action action);

  [[nodiscard]] const olc::Sprite* GetSprite(EntityId id) const;

 private:
  std::map<SpriteKey, AnimatedSprite> animated_sprites_;
  std::shared_ptr<Registry> registry_;
};
}  // namespace platformer