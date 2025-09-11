#pragma once

#include <map>
#include <set>

#include "animated_sprite.h"

namespace platformer {

enum class Action { Idle, Walk, Shoot, Jump };

struct ActionSpriteSheet {
  Action action;
  AnimatedSprite sprite;
};

class AnimationManager {
 public:
  AnimationManager() = default;  // REMOVE
  AnimationManager(std::vector<ActionSpriteSheet> actions);

  void StartAction(Action action);

  void EndAction(Action action);

  const olc::Sprite* GetSprite();

 private:
  void RemoveExpiredActions();

  std::map<Action, AnimatedSprite> animated_sprites_;
  std::set<Action> active_actions_;
};
}  // namespace platformer