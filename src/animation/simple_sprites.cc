#include "simple_sprites.h"

#include "animated_sprite.h"
#include "utils/check.h"

namespace platformer {

std::unique_ptr<olc::Sprite> CreateShotgunPelletSprite() {
  auto sprite = std::make_unique<olc::Sprite>(3, 3);
  sprite->SetPixel(0, 1, olc::WHITE);
  sprite->SetPixel(1, 0, olc::WHITE);
  sprite->SetPixel(1, 1, olc::WHITE);
  sprite->SetPixel(1, 2, olc::WHITE);
  sprite->SetPixel(2, 1, olc::WHITE);

  return sprite;
}

// AnimatedSprite CreateSmallWallHitEffectSprite(){
// }

}  // namespace platformer