#include "animation_manager.h"

#include <optional>

#include "animation/animated_sprite.h"
#include "common_types/actor_state.h"
#include "common_types/entity.h"
#include "registry_helpers.h"
#include "utils/check.h"
#include "utils/logging.h"

namespace platformer {

  namespace {
    std::string MakeKey(const Actor actor, const State state) {
      return ToString(actor) + "-" + ToString(state);
    }
  }

void AnimationManager::AddAnimation(AnimatedSprite sprite, Actor actor, State state) {
  animated_sprites_.try_emplace(MakeKey(actor, state), std::move(sprite));
}

void AnimationManager::AddAnimation(AnimatedSprite sprite, const std::string& key){
  animated_sprites_.try_emplace(key, std::move(sprite));
}

AnimatedSprite& AnimationManager::GetAnimation(Actor actor, State state) {
  return animated_sprites_.at(MakeKey(actor, state));
}

const AnimatedSprite& AnimationManager::GetAnimation(Actor actor, State state) const {
  return animated_sprites_.at(MakeKey(actor, state));
}

void AnimationManager::AddInsideSpriteLocation(InsideSpriteLocation location,
                                               Actor actor,
                                               State state) {
  inside_sprite_locations_.try_emplace(MakeKey(actor, state), std::move(location));
}

std::optional<InsideSpriteLocation> AnimationManager::GetInsideSpriteLocation(
    const EntityId entity_id) const {
  if (!registry_->HasComponent<StateComponent>(entity_id)) {
    return std::nullopt;
  }
  const auto& state = registry_->GetComponent<StateComponent>(entity_id);
  const auto itr =  inside_sprite_locations_.find(MakeKey(state.actor_type, state.state.GetState()));
  if(itr == inside_sprite_locations_.end()) {
    return std::nullopt;
  }
  return itr->second;
}

std::vector<AnimationEvent> AnimationManager::GetAnimationEvents() const {
  std::vector<AnimationEvent> events;

  for (EntityId id : registry_->GetView<StateComponent>()) {
    // TODO(UL-13) WHY is this function allowed to be const?!
    auto& state = registry_->GetComponent<StateComponent>(id);
    const auto& animated_sprite =
        animated_sprites_.at(MakeKey(state.actor_type, state.state.GetState()));

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
  RB_CHECK(registry_->HasComponent<Animation>(id));
  if(registry_->HasComponent<StateComponent>(id)) {
    const auto& state = registry_->GetComponent<StateComponent>(id);
    const auto& animated_sprite =
        animated_sprites_.at(MakeKey(state.actor_type, state.state.GetState()));
    return animated_sprite.GetFrame(state.state.GetStateSetAt());
  }
  const auto animation = registry_->GetComponent<Animation>(id);
  const auto &animated_sprite = animated_sprites_.at(animation.key);
  return animated_sprite.GetFrame(animation.start_time);
}

}  // namespace platformer