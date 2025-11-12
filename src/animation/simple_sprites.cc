#include "simple_sprites.h"

#include "animated_sprite.h"
#include "utils/check.h"

namespace platformer {

  AnimatedSprite CreateShotgunPelletSprite(){
    std::vector<std::unique_ptr<olc::Sprite>> frames;
    frames.emplace_back(std::make_unique<olc::Sprite>(3, 3));
    auto& sprite = *frames.back();
    sprite.SetPixel(0, 1, olc::WHITE);
    sprite.SetPixel(1, 0, olc::WHITE);
    sprite.SetPixel(1, 1, olc::WHITE);
    sprite.SetPixel(1, 2, olc::WHITE);
    sprite.SetPixel(2, 1, olc::WHITE);


    auto animated_sprite = AnimatedSprite::CreateAnimatedSprite(std::move(frames), std::vector<int>{100}, true);
    RB_CHECK(animated_sprite.has_value());
    return std::move(*animated_sprite);
  }

  // AnimatedSprite CreateSmallWallHitEffectSprite(){
  // }

}