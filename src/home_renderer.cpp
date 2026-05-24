#include "home_renderer.h"

#include <M5Unified.h>

namespace homedeck {

void HomeRenderer::render() {
  M5.Display.setRotation(0);
  M5.Display.fillScreen(TFT_WHITE);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextSize(2);
  M5.Display.setTextDatum(textdatum_t::middle_center);
  M5.Display.drawString("HomeDeck", M5.Display.width() / 2, M5.Display.height() / 2);
}

}  // namespace homedeck
