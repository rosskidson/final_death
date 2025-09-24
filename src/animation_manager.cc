#include "animation_manager.h"

#include <cassert>

#include "animated_sprite.h"
#include "player_state.h"
#include "utils/logging.h"

namespace platformer {

void AnimationManager::AddAnimation(AnimatedSprite sprite, const PlayerState action) {
  animated_sprites_.try_emplace(action, std::move(sprite));
}

const AnimatedSprite& AnimationManager::GetAnimation(const PlayerState action) const {
  return animated_sprites_.at(action);
}

AnimatedSprite& AnimationManager::GetAnimation(const PlayerState action) {
  return animated_sprites_.at(action);
}

const AnimatedSprite& AnimationManager::GetActiveAnimation() const {
  return animated_sprites_.at(active_action_);
}

AnimatedSprite& AnimationManager::GetActiveAnimation() {
  return animated_sprites_.at(active_action_);
}

void AnimationManager::Update(const PlayerState new_action) {
  // Trigger callbacks before changing away from an expired animation.
  GetActiveAnimation().TriggerCallbacks();
  if (new_action != active_action_ || GetActiveAnimation().Expired()) {
    LOG_INFO("old action: " << ToString(active_action_)
                            << ", new action: " << ToString(new_action)
                            << ", expired: " << GetActiveAnimation().Expired());
    active_action_ = new_action;
    GetActiveAnimation().StartAnimation();
  }
}

void AnimationManager::SwapAnimation(PlayerState action) {
  const auto begin_time = GetActiveAnimation().GetStartTime();
  active_action_ = action;
  GetActiveAnimation().StartAnimation(begin_time);
}

const olc::Sprite* AnimationManager::GetSprite() const { return GetActiveAnimation().GetFrame(); }

}  // namespace platformer