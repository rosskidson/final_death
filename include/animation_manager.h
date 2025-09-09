#pragma once

#include <map>
#include <set>

#include "animated_sprite.h"
#include "basic_types.h"

namespace platformer {

enum class Action { Walk, Shoot, Jump };

struct ActionSpriteSheetPath {
  Action action;
  std::string path;
  bool looping;
};

class AnimationManager {
 public:
  AnimationManager(std::vector<ActionSpriteSheetPath>& spritesheet_paths);

  void StartAction(Action action);

  void EndAction(Action action);

  olc::Sprite* GetSprite();

 private:
  std::map<Action, AnimatedSprite> animated_sprites_;
  std::set<Action> active_actions_;
};
}  // namespace platformer