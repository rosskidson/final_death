#include "animation_manager.h"

namespace platformer {

AnimationManager::AnimationManager(std::vector<ActionSpriteSheet> actions) {
  for (auto& action : actions) {
    animated_sprites_[action.action] = std::move(action.sprite);
  }
  animated_sprites_[Action::Idle].StartAnimation();
}

void AnimationManager::StartAction(const Action action) {
  active_actions_.insert(action);
  animated_sprites_[action].StartAnimation();
}

void AnimationManager::EndAction(const Action action) { active_actions_.erase(action); }

const olc::Sprite* AnimationManager::GetSprite() {
  RemoveExpiredActions();
  if (active_actions_.count(Action::Shoot) != 0) {
    return animated_sprites_[Action::Shoot].GetFrame();
  }
  if (active_actions_.count(Action::Jump) != 0) {
    return animated_sprites_[Action::Jump].GetFrame();
  }
  if (active_actions_.count(Action::Roll) != 0) {
    return animated_sprites_[Action::Roll].GetFrame();
  }
  if (active_actions_.count(Action::Walk) != 0) {
    return animated_sprites_[Action::Walk].GetFrame();
  }
  if (active_actions_.count(Action::Crouch) != 0) {
    return animated_sprites_[Action::Crouch].GetFrame();
  }
  return animated_sprites_[Action::Idle].GetFrame();
}

void AnimationManager::RemoveExpiredActions() {
  std::vector<Action> actions_to_remove;
  for (const auto& action : active_actions_) {
    if (animated_sprites_[action].Expired()) {
      actions_to_remove.push_back(action);
    }
  }
  for (const auto& action : actions_to_remove) {
    active_actions_.erase(action);
  }
  if (!actions_to_remove.empty() && active_actions_.empty()) {
    animated_sprites_[Action::Idle].StartAnimation();
  }
}

}  // namespace platformer