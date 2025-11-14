#include "animation_manager.h"

#include <optional>

#include "animation/animated_sprite.h"
#include "common_types/actor_state.h"
#include "common_types/entity.h"
#include "registry_helpers.h"
#include "utils/check.h"
#include "utils/logging.h"

namespace platformer {

void AnimationManager::AddAnimation(const std::string& key, AnimatedSprite sprite) {
  animated_sprites_.try_emplace(key, std::move(sprite));
}

AnimatedSprite& AnimationManager::GetAnimation(const std::string& key) {
  return animated_sprites_.at(key);
}

const AnimatedSprite& AnimationManager::GetAnimation(const std::string& key) const {
  return animated_sprites_.at(key);
}

void AnimationManager::AddInsideSpriteLocation(const std::string& key,
                                               InsideSpriteLocation location) {
  inside_sprite_locations_.try_emplace(key, std::move(location));
}

std::optional<InsideSpriteLocation> AnimationManager::GetInsideSpriteLocation(
    const EntityId entity_id) const {
  if (!registry_->HasComponent<AnimatedSpriteComponent>(entity_id)) {
    return std::nullopt;
  }
  const auto& component = registry_->GetComponent<AnimatedSpriteComponent>(entity_id);
  const auto itr =  inside_sprite_locations_.find(component.key);
  if(itr == inside_sprite_locations_.end()) {
    return std::nullopt;
  }
  return itr->second;
}

std::vector<AnimationEvent> AnimationManager::GetAnimationEvents() {
  std::vector<AnimationEvent> events;

  for (EntityId id : registry_->GetView<AnimatedSpriteComponent>()) {
    auto& animated_sprite_component = registry_->GetComponent<AnimatedSpriteComponent>(id);
    const auto& animated_sprite =
        animated_sprites_.at(animated_sprite_component.key);

    for (const auto& event_name :
        animated_sprite.GetAnimationEvents(animated_sprite_component.start_time, 
                                           animated_sprite_component.last_animation_frame_idx)) {
      events.push_back(AnimationEvent{id, animated_sprite_component.key, event_name});
    }
  }
  return events;
}

Sprite AnimationManager::GetSprite(EntityId id) const {
  RB_CHECK(registry_->HasComponent<AnimatedSpriteComponent>(id) || 
           registry_->HasComponent<SpriteComponent>(id));
  RB_CHECK(registry_->HasComponent<AnimatedSpriteComponent>(id) &&
           registry_->HasComponent<SpriteComponent>(id));

  if(registry_->HasComponent<AnimatedSpriteComponent>(id)) {
    const auto &animated_sprite_component = registry_->GetComponent<AnimatedSpriteComponent>(id);
    const auto &animated_sprite = animated_sprites_.at(animated_sprite_component.key);
    return animated_sprite.GetFrame(animated_sprite_component.start_time);
  }

  // TODO:: implement sprite component
  return Sprite{};
}

}  // namespace platformer