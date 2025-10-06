#include "animation_manager.h"

#include "animation/animated_sprite.h"
#include "common_types/actor_state.h"
#include "registry_helpers.h"
#include "utils/check.h"
#include "utils/logging.h"

namespace platformer {

// void AnimationManager::Update(EntityId id) {
//   if (animated_sprites_.count(new_action) == 0) {
//     LOG_ERROR("No animation available for '" << ToString(new_action) << "'");
//     return;
//   }

//   // Trigger callbacks before changing away from an expired animation.
//   GetActiveAnimation().TriggerCallbacks();
//   if (new_action != active_action_ || GetActiveAnimation().Expired()) {
//     // LOG_INFO("old action: " << ToString(active_action_)
//     //                         << ", new action: " << ToString(new_action)
//     //                         << ", expired: " << GetActiveAnimation().Expired());
//     active_action_ = new_action;
//     GetActiveAnimation().StartAnimation();
//     GetActiveAnimation().TriggerCallbacks();
//   }
// }

// void AnimationManager::SwapAnimation(PlayerState action) {
//   if (animated_sprites_.count(action) == 0) {
//     LOG_ERROR("No animation available for '" << ToString(action) << "'");
//     return;
//   }

//   auto begin_time = GetActiveAnimation().GetStartTime();
//   const auto& new_animation = animated_sprites_.at(action);

//   // If the new action is shorter than the old one, we must handle the difference to expire the
//   new
//   // one at the appropriate time.
//   // I hate this function.
//   const int duration_difference =
//       GetActiveAnimation().GetTotalAnimationTimeMs() - new_animation.GetTotalAnimationTimeMs();
//   if (duration_difference > 0) {
//     begin_time += std::chrono::milliseconds(duration_difference);
//   }
//   active_action_ = action;
//   GetActiveAnimation().StartAnimation(begin_time);
// }

// TODO:: Remove const
std::vector<AnimationEvent> AnimationManager::GetAnimationEvents() const {
  std::vector<AnimationEvent> events;

  for (EntityId id : registry_->GetView<StateComponent>()) {
    auto& state = registry_->GetComponent<StateComponent>(id);
    const auto& animated_sprite =
        animated_sprites_.at(SpriteKey{state.actor_type, state.state.GetState()});

    const auto start_time = state.state.GetStateSetAt();
    auto animation_frame_idx = state.state.GetLastAnimationFrameIdx();

    for (const auto& event_name :
         animated_sprite.GetAnimationEvents(start_time, animation_frame_idx)) {
      events.push_back(AnimationEvent{id, state.state.GetState(), event_name});
    }
    state.state.SetLastAnimationFrameIdx(animation_frame_idx);
  }
  return events;
}

const olc::Sprite* AnimationManager::GetSprite(EntityId id) const {
  RB_CHECK(registry_->HasComponent<StateComponent>(id));
  const auto& state = registry_->GetComponent<StateComponent>(id);
  const auto& animated_sprite =
      animated_sprites_.at(SpriteKey{state.actor_type, state.state.GetState()});
  return animated_sprite.GetFrame(state.state.GetStateSetAt());
}

}  // namespace platformer