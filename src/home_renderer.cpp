#include "home_renderer.h"

#include <LittleFS.h>
#include <M5Unified.h>
#include <qrcode.h>

#include <cstdint>
#include <string>
#include <vector>

#include "generated/device_font_vlw.h"

namespace homedeck {
namespace {

constexpr int kLogoTopY = 86;
constexpr int kLogoHeight = 40;
constexpr int kLogoCenterY = kLogoTopY + kLogoHeight / 2;
constexpr int kTextFrameHeight = 27;
constexpr int kApTextTopY = kLogoTopY + kLogoHeight + 26;
constexpr int kApTextCenterY = kApTextTopY + kTextFrameHeight / 2;
constexpr int kIpTextTopY = kApTextTopY + kTextFrameHeight + 26;
constexpr int kIpTextCenterY = kIpTextTopY + kTextFrameHeight / 2;
constexpr int kQrLeftX = 72;
constexpr int kQrTopY = kIpTextTopY + kTextFrameHeight + 26;
constexpr int kQrSize = 256;

int centerX() {
  return M5.Display.width() / 2;
}

void drawLogo(int y) {
  if (!LittleFS.begin()) {
    return;
  }
  M5.Display.drawPngFile(
      LittleFS,
      "/logo.png",
      centerX(),
      y,
      0,
      0,
      0,
      0,
      1.0f,
      1.0f,
      datum_t::middle_center);
  LittleFS.end();
}

void prepareScreen() {
  M5.Display.setRotation(0);
  M5.Display.fillScreen(TFT_WHITE);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextDatum(textdatum_t::middle_center);
}

void loadConfigPortalFont() {
  if (!M5.Display.loadFont(generated::kConfigPortalFontVlw)) {
    M5.Display.setFont(nullptr);
  }
  M5.Display.setTextSize(1);
}

}  // namespace

void HomeRenderer::render() {
  prepareScreen();
  drawLogo(M5.Display.height() / 2);
}

void HomeRenderer::renderConfigPortal(const std::string& apSsid, const std::string& ipAddress) {
  prepareScreen();

  drawLogo(kLogoCenterY);

  loadConfigPortalFont();
  M5.Display.drawString(apSsid.c_str(), centerX(), kApTextCenterY);
  M5.Display.drawString(ipAddress.c_str(), centerX(), kIpTextCenterY);
  M5.Display.unloadFont();

  const std::string qrText = std::string("WIFI:T:nopass;S:") + apSsid + ";;";
  drawQrCode(qrText, kQrLeftX, kQrTopY, kQrSize);
}

void HomeRenderer::drawQrCode(const std::string& text, int left, int top, int size) {
  QRCode qrcode;
  std::vector<std::uint8_t> qrcodeBuffer(qrcode_getBufferSize(3));
  qrcode_initText(&qrcode, qrcodeBuffer.data(), 3, ECC_LOW, text.c_str());

  for (int y = 0; y < qrcode.size; ++y) {
    for (int x = 0; x < qrcode.size; ++x) {
      if (!qrcode_getModule(&qrcode, x, y)) {
        continue;
      }

      const int moduleLeft = left + x * size / qrcode.size;
      const int moduleTop = top + y * size / qrcode.size;
      const int moduleRight = left + (x + 1) * size / qrcode.size;
      const int moduleBottom = top + (y + 1) * size / qrcode.size;
      M5.Display.fillRect(
          moduleLeft,
          moduleTop,
          moduleRight - moduleLeft,
          moduleBottom - moduleTop,
          TFT_BLACK);
    }
  }
}

}  // namespace homedeck
