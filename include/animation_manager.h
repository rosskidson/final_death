#pragma once

#include <map>
#include <set>

#include "animated_sprite.h"

namespace platformer {

enum class Action { Idle, Walk, Shoot, Jump, Crouch, Roll };

class AnimationManager {
 public:
  AnimationManager() = default;  // REMOVE
  void AddAnimation(AnimatedSprite sprite, Action action);

  AnimatedSprite& GetAnimation(Action action);

  [[nodiscard]] AnimatedSprite& GetActiveAnimation() ; // TODO:: const

  void StartAction(Action action);

  void EndAction(Action action);

  const olc::Sprite* GetSprite();  // TODO:: const

 private:
  void RemoveExpiredActions();

  std::map<Action, AnimatedSprite> animated_sprites_;
  std::set<Action> active_actions_;
};
}  // namespace platformer