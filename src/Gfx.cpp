#include "Managers.h"
#include "Config.h"

namespace {
M5Canvas canvas(&M5.Display);
}

namespace Gfx {

void init() {
  canvas.setPsram(true);
  canvas.setColorDepth(16);
  canvas.createSprite(SCREEN_W, SCREEN_H);
  canvas.setTextDatum(top_left);
  canvas.fillScreen(TFT_BLACK);
}

M5Canvas& c() {
  return canvas;
}

void present() {
  canvas.pushSprite(0, 0);
}

}
