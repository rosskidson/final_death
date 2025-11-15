#include "sprite_manager.h"

#include <optional>

#include "animation/animated_sprite.h"
#include "common_types/components.h"
#include "common_types/entity.h"
#include "utils/check.h"
#include "utils/logging.h"

namespace platformer {

void SpriteManager::AddAnimation(const std::string& key, AnimatedSprite sprite) {
  animated_sprites_.try_emplace(key, std::move(sprite));
}

void SpriteManager::AddSprite(const std::string& key,
                              const int draw_offset_x,
                              const int draw_offset_y,
                              std::unique_ptr<olc::Sprite> sprite) {
  SpriteStorage s{std::move(sprite), draw_offset_x, draw_offset_y};
  sprites_.try_emplace(key, std::move(s));
}

Sprite SpriteManager::GetSprite(const std::string& key) const {
  const auto& sprite = sprites_.at(key);
  return {sprite.sprite.get(), sprite.draw_offset_x, sprite.draw_offset_y};
}

AnimatedSprite& SpriteManager::GetAnimation(const std::string& key) {
  return animated_sprites_.at(key);
}

const AnimatedSprite& SpriteManager::GetAnimation(const std::string& key) const {
  return animated_sprites_.at(key);
}

void SpriteManager::AddInsideSpriteLocation(const std::string& key, InsideSpriteLocation location) {
  inside_sprite_locations_.try_emplace(key, location);
}

std::optional<InsideSpriteLocation> SpriteManager::GetInsideSpriteLocation(
    const EntityId entity_id) const {
  if (!registry_->HasComponent<AnimatedSpriteComponent>(entity_id)) {
    return std::nullopt;
  }
  const auto& component = registry_->GetComponent<AnimatedSpriteComponent>(entity_id);
  const auto itr = inside_sprite_locations_.find(component.key);
  if (itr == inside_sprite_locations_.end()) {
    return std::nullopt;
  }
  return itr->second;
}

std::vector<AnimationEvent> SpriteManager::GetAnimationEvents() {
  std::vector<AnimationEvent> events;

  for (EntityId id : registry_->GetView<AnimatedSpriteComponent>()) {
    auto& animated_sprite_component = registry_->GetComponent<AnimatedSpriteComponent>(id);
    const auto& animated_sprite = animated_sprites_.at(animated_sprite_component.key);

    for (const auto& event_name :
         animated_sprite.GetAnimationEvents(animated_sprite_component.start_time,
                                            animated_sprite_component.last_animation_frame_idx)) {
      events.push_back(AnimationEvent{id, animated_sprite_component.key, event_name});
    }
  }
  return events;
}

Sprite SpriteManager::GetSprite(EntityId id) const {
  RB_CHECK(registry_->HasComponent<AnimatedSpriteComponent>(id) ||
           registry_->HasComponent<SpriteComponent>(id));

  if (registry_->HasComponent<AnimatedSpriteComponent>(id)) {
    const auto& animated_sprite_component = registry_->GetComponent<AnimatedSpriteComponent>(id);
    const auto& animated_sprite = animated_sprites_.at(animated_sprite_component.key);
    return animated_sprite.GetFrame(animated_sprite_component.start_time);
  }

  return GetSprite(registry_->GetComponent<SpriteComponent>(id).key);
}

}  // namespace platformer