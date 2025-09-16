#include "animation_manager.h"

namespace platformer {

void AnimationManager::AddAnimation(AnimatedSprite sprite, Action action) {
  animated_sprites_.try_emplace(action, std::move(sprite));
}

void AnimationManager::StartAction(const Action action) {
  active_actions_.insert(action);
  animated_sprites_.at(action).StartAnimation();
}

void AnimationManager::EndAction(const Action action) { active_actions_.erase(action); }

const olc::Sprite* AnimationManager::GetSprite() {
  RemoveExpiredActions();
  if (active_actions_.count(Action::Shoot) != 0) {
    return animated_sprites_.at(Action::Shoot).GetFrame();
  }
  if (active_actions_.count(Action::Jump) != 0) {
    return animated_sprites_.at(Action::Jump).GetFrame();
  }
  if (active_actions_.count(Action::Roll) != 0) {
    return animated_sprites_.at(Action::Roll).GetFrame();
  }
  if (active_actions_.count(Action::Walk) != 0) {
    return animated_sprites_.at(Action::Walk).GetFrame();
  }
  if (active_actions_.count(Action::Crouch) != 0) {
    return animated_sprites_.at(Action::Crouch).GetFrame();
  }
  return animated_sprites_.at(Action::Idle).GetFrame();
}

void AnimationManager::RemoveExpiredActions() {
  std::vector<Action> actions_to_remove;
  for (const auto& action : active_actions_) {
    if (animated_sprites_.at(action).Expired()) {
      actions_to_remove.push_back(action);
    }
  }
  for (const auto& action : actions_to_remove) {
    active_actions_.erase(action);
  }
  if (!actions_to_remove.empty() && active_actions_.empty()) {
    animated_sprites_.at(Action::Idle).StartAnimation();
  }
}

}  // namespace platformer