#pragma once

namespace olc {
class Sprite;
}

namespace platformer {

struct Sprite {
  const olc::Sprite* sprite_ptr;
  // Offset is measured from the bottom right
  int draw_offset_x{};
  int draw_offset_y{};
};

}  // namespace platformer