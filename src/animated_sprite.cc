
#include "animated_sprite.h"

#include <utils/game_clock.h>

#include <cassert>
#include <memory>

namespace platformer {

AnimatedSprite::AnimatedSprite(const std::string& sprite_sheet_path,
                               int sprite_width,
                               bool loops,
                               int frame_delay_ms)
    : loops_(loops), frame_delay_ms_(frame_delay_ms) {
  assert(frame_delay_ms_ > 0);
  olc::Sprite spritesheet_img{};
  if (spritesheet_img.LoadFromFile(sprite_sheet_path) != olc::rcode::OK) {
    std::cout << "Failed loading sprite " << std::endl;
    // TODO:: Private constructor and public creator pattern
    exit(1);
  }

  // The spritesheet is assumed to have all the sprites packed tightly on one horizontal line.
  // Check the spritesheet width is divisible by the sprite width.
  assert(spritesheet_img.width % sprite_width == 0);

  frame_count_ = spritesheet_img.width / sprite_width;
  const int sprite_height = spritesheet_img.height;

  for (int frame_idx = 0; frame_idx < frame_count_; ++frame_idx) {
    auto sprite = std::make_unique<olc::Sprite>(sprite_width, sprite_height);
    for (int j = 0; j < sprite_height; ++j) {
      for (int i = 0; i < sprite_width; ++i) {
        sprite->SetPixel(i, j, spritesheet_img.GetPixel(frame_idx * sprite_width + i, j));
      }
    }
    frames_.emplace_back(std::move(sprite));
  }
}

void AnimatedSprite::StartAnimation() { start_time_ = GameClock::NowGlobal(); }

bool AnimatedSprite::Expired() const {
  if (loops_) {
    return false;
  }
  const auto time_elapsed = GameClock::NowGlobal() - start_time_;
  return (time_elapsed.count() / 1000000) > (frame_count_ * frame_delay_ms_);
}

olc::Sprite* AnimatedSprite::GetFrame() {
  if (Expired()) {
    return nullptr;
  }
  const int time_elapsed = static_cast<int>((GameClock::NowGlobal() - start_time_).count() / 1e6);
  const int frame_idx = (time_elapsed / frame_delay_ms_) % frame_count_;
  return frames_.at(frame_idx).get();
}

}  // namespace platformer