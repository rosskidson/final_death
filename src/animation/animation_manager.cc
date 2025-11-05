#include "animation_manager.h"

#include <optional>

#include "animation/animated_sprite.h"
#include "common_types/actor_state.h"
#include "common_types/entity.h"
#include "registry_helpers.h"
#include "utils/check.h"
#include "utils/logging.h"

namespace platformer {

void AnimationManager::AddAnimation(AnimatedSprite sprite, Actor actor, State state) {
  animated_sprites_.try_emplace(SpriteKey{actor, state}, std::move(sprite));
}

AnimatedSprite& AnimationManager::GetAnimation(Actor actor, State state) {
  return animated_sprites_.at(SpriteKey{actor, state});
}

const AnimatedSprite& AnimationManager::GetAnimation(Actor actor, State state) const {
  return animated_sprites_.at(SpriteKey{actor, state});
}

void AnimationManager::AddInsideSpriteLocation(InsideSpriteLocation location,
                                               Actor actor,
                                               State state) {
  inside_sprite_locations_.try_emplace(SpriteKey{actor, state}, std::move(location));
}

std::optional<InsideSpriteLocation> AnimationManager::GetInsideSpriteLocation(
    const EntityId entity_id) const {
  if (!registry_->HasComponent<StateComponent>(entity_id)) {
    return std::nullopt;
  }
  const auto& state = registry_->GetComponent<StateComponent>(entity_id);
  const auto itr =  inside_sprite_locations_.find(SpriteKey{state.actor_type, state.state.GetState()});
  if(itr == inside_sprite_locations_.end()) {
    return std::nullopt;
  }
  return itr->second;
}

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