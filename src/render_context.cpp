#include "render_context.h"

namespace homedeck {

void prepareScreen(M5Canvas& canvas) {
  canvas.fillSprite(TFT_WHITE);
  canvas.setTextColor(TFT_BLACK, TFT_WHITE);
  canvas.setTextDatum(textdatum_t::middle_center);
}

void pushScreen(M5Canvas& canvas) {
  canvas.pushSprite(0, 0);
  M5.Display.waitDisplay();
}

M5Canvas& sprite() {
  static M5Canvas canvas(&M5.Display);
  static bool ready = false;
  if (!ready) {
    canvas.setColorDepth(16);
    canvas.createSprite(M5.Display.width(), M5.Display.height());
    ready = true;
  }
  return canvas;
}

}  // namespace homedeck
