
#include "animated_sprite.h"

#include <utils/game_clock.h>

#include <cassert>
#include <memory>
#include <optional>

namespace platformer {

std::optional<AnimatedSprite> AnimatedSprite::CreateAnimatedSprite(
    const std::filesystem::path& sprite_sheet_path,
    int sprite_width,
    bool loops,
    bool forwards_backwards,
    int animation_duration_ms) {
  AnimatedSprite animated_sprite{};
  animated_sprite.loops_ = loops;
  animated_sprite.forwards_backwards_ = forwards_backwards;

  olc::Sprite spritesheet_img{};
  if (spritesheet_img.LoadFromFile(sprite_sheet_path.string()) != olc::rcode::OK) {
    std::cerr << "Failed loading sprite " << std::endl;
    return std::nullopt;
  }

  // The spritesheet is assumed to have all the sprites packed tightly on one horizontal line.
  // Check the spritesheet width is divisible by the sprite width.
  if (spritesheet_img.width % sprite_width != 0) {
    std::cerr << "Failed loading sprite sheet. The total width must be divisible by the sprite "
                 "width, otherwise the frames will load misaligned. The total width is "
              << spritesheet_img.width << " and the requested width is " << sprite_width
              << std::endl;
    return std::nullopt;
  }

  animated_sprite.frame_count_ = spritesheet_img.width / sprite_width;
  if (animation_duration_ms <= 0) {
    std::cerr << "Animation duration must be greater than 0" << std::endl;
    return std::nullopt;
  }
  if (forwards_backwards) {
    animated_sprite.frame_delay_ms_ =
        animation_duration_ms / (animated_sprite.frame_count_ * 2 - 2);
  } else {
    animated_sprite.frame_delay_ms_ = animation_duration_ms / animated_sprite.frame_count_;
  }
  const int sprite_height = spritesheet_img.height;

  for (int frame_idx = 0; frame_idx < animated_sprite.frame_count_; ++frame_idx) {
    auto sprite = std::make_unique<olc::Sprite>(sprite_width, sprite_height);
    for (int j = 0; j < sprite_height; ++j) {
      for (int i = 0; i < sprite_width; ++i) {
        sprite->SetPixel(i, j, spritesheet_img.GetPixel(frame_idx * sprite_width + i, j));
      }
    }
    animated_sprite.frames_.emplace_back(std::move(sprite));
  }
  return animated_sprite;
}

void AnimatedSprite::StartAnimation() { start_time_ = GameClock::NowGlobal(); }

bool AnimatedSprite::Expired() const {
  if (loops_) {
    return false;
  }
  const auto time_elapsed = GameClock::NowGlobal() - start_time_;
  return (time_elapsed.count() / 1000000) > (frame_count_ * frame_delay_ms_);
}

const olc::Sprite* AnimatedSprite::GetFrame() const {
  if (Expired()) {
    return nullptr;
  }
  const int time_elapsed = static_cast<int>((GameClock::NowGlobal() - start_time_).count() / 1e6);
  if (!forwards_backwards_) {
    const int frame_idx = (time_elapsed / frame_delay_ms_) % frame_count_;
    return frames_.at(frame_idx).get();
  }
  const int frame_idx = (time_elapsed / frame_delay_ms_) % (2 * frame_count_ - 2);
  if (frame_idx < frame_count_) {
    return frames_.at(frame_idx).get();
  }
  return frames_.at(2 * frame_count_ - frame_idx - 2).get();
}

}  // namespace platformer