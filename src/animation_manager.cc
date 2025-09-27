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
  if (animated_sprites_.count(new_action) == 0) {
    LOG_ERROR("No animation available for '" << ToString(new_action) << "'");
    return;
  }

  // Trigger callbacks before changing away from an expired animation.
  GetActiveAnimation().TriggerCallbacks();
  if (new_action != active_action_ || GetActiveAnimation().Expired()) {
    // LOG_INFO("old action: " << ToString(active_action_)
    //                         << ", new action: " << ToString(new_action)
    //                         << ", expired: " << GetActiveAnimation().Expired());
    active_action_ = new_action;
    GetActiveAnimation().StartAnimation();
    GetActiveAnimation().TriggerCallbacks();
  }
}

void AnimationManager::SwapAnimation(PlayerState action) {
  if (animated_sprites_.count(action) == 0) {
    LOG_ERROR("No animation available for '" << ToString(action) << "'");
    return;
  }

  auto begin_time = GetActiveAnimation().GetStartTime();
  const auto& new_animation = animated_sprites_.at(action);

  // If the new action is shorter than the old one, we must handle the difference to expire the new
  // one at the appropriate time.
  // I hate this function.
  const int duration_difference =
      GetActiveAnimation().GetTotalAnimationTimeMs() - new_animation.GetTotalAnimationTimeMs();
  if (duration_difference > 0) {
    begin_time += std::chrono::milliseconds(duration_difference);
  }
  active_action_ = action;
  GetActiveAnimation().StartAnimation(begin_time);
}

const olc::Sprite* AnimationManager::GetSprite() const { return GetActiveAnimation().GetFrame(); }

}  // namespace platformer