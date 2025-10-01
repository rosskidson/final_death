#include "olcPixelGameEngine.h"
#include "platformer.h"

int main() {
  platformer::Platformer platformer{};

  if (platformer.Start() == olc::rcode::FAIL) {
    return -1;
  }
  return 0;
}
