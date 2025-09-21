#include "animation_manager.h"
#include "utils/logging.h"
#include "animated_sprite.h"

namespace platformer {

void AnimationManager::AddAnimation(AnimatedSprite sprite, Action action) {
  animated_sprites_.try_emplace(action, std::move(sprite));
}

AnimatedSprite& AnimationManager::GetAnimation(Action action) {
  return animated_sprites_.at(action);
}

// TODO:: lol obvious fix this shit.
Action AnimationManager::GetActiveAction() const {
  if (active_actions_.count(Action::Shoot) != 0) {
    return Action::Shoot;
  }
  if (active_actions_.count(Action::JumpCrouch) != 0) {
    return Action::JumpCrouch;
  }
  if (active_actions_.count(Action::Fly) != 0) {
    return Action::Fly;
  }
  if (active_actions_.count(Action::Roll) != 0) {
    return Action::Roll;
  }
  if (active_actions_.count(Action::Walk) != 0) {
    return Action::Walk;
  }
  if (active_actions_.count(Action::Crouch) != 0) {
    return Action::Crouch;
  }
  return Action::Idle;
}

AnimatedSprite& AnimationManager::GetActiveAnimation() {
  RemoveExpiredActions();
  if (active_actions_.count(Action::Shoot) != 0) {
    return animated_sprites_.at(Action::Shoot);
  }
  if (active_actions_.count(Action::JumpCrouch) != 0) {
    return animated_sprites_.at(Action::JumpCrouch);
  }
  if (active_actions_.count(Action::Fly) != 0) {
    return animated_sprites_.at(Action::Fly);
  }
  if (active_actions_.count(Action::Roll) != 0) {
    return animated_sprites_.at(Action::Roll);
  }
  if (active_actions_.count(Action::Walk) != 0) {
    return animated_sprites_.at(Action::Walk);
  }
  if (active_actions_.count(Action::Crouch) != 0) {
    return animated_sprites_.at(Action::Crouch);
  }
  return animated_sprites_.at(Action::Idle);
}

void AnimationManager::StartAction(const Action action) {
  active_actions_.insert(action);
  animated_sprites_.at(action).StartAnimation();
}

void AnimationManager::EndAction(const Action action) { active_actions_.erase(action); }

const olc::Sprite* AnimationManager::GetSprite() { return GetActiveAnimation().GetFrame(); }

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