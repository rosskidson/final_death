#pragma once

#include <memory>
#include "animated_sprite.h"

namespace platformer {

std::unique_ptr<olc::Sprite> CreateShotgunPelletSprite();
AnimatedSprite CreateSmallWallHitEffectSprite();

}  // namespace platformer