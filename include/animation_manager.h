#pragma once

#include <map>

#include "animated_sprite.h"
#include "player_state.h"

namespace platformer {

class AnimationManager {
 public:
  AnimationManager() : active_action_(PlayerState::Idle) {}

  void AddAnimation(AnimatedSprite sprite, PlayerState action);

  [[nodiscard]] const AnimatedSprite& GetAnimation(PlayerState action) const;
  [[nodiscard]] AnimatedSprite& GetAnimation(PlayerState action);

  [[nodiscard]] const AnimatedSprite& GetActiveAnimation() const;
  [[nodiscard]] AnimatedSprite& GetActiveAnimation();

  void SwapAnimation(PlayerState action);

  void Update(PlayerState new_action);

  // void StartAction(Action action);
  // void AddAction(Action action);
  // void EndAction(Action action);

  [[nodiscard]] const olc::Sprite* GetSprite() const;

 private:
  std::map<PlayerState, AnimatedSprite> animated_sprites_;
  PlayerState active_action_;
};
}  // namespace platformer