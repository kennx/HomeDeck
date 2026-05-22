#include "setup_renderer.h"

#include <M5Unified.h>
#include <qrcode.h>

#include <string>

#include "homedeck/setup_page.h"

namespace {

void drawQrCode(const std::string& payload) {
  QRCode qrcode;
  uint8_t modules[qrcode_getBufferSize(4)];
  qrcode_initText(&qrcode, modules, 4, ECC_LOW, payload.c_str());

  constexpr int kScale = 6;
  constexpr int kQuietZone = 2;
  const int qrSize = qrcode.size;
  const int totalSize = (qrSize + kQuietZone * 2) * kScale;
  const int originX = M5.Display.width() - totalSize - 20;
  const int originY = 120;

  M5.Display.fillRect(originX, originY, totalSize, totalSize, TFT_WHITE);

  for (int y = 0; y < qrSize; ++y) {
    for (int x = 0; x < qrSize; ++x) {
      if (!qrcode_getModule(&qrcode, x, y)) {
        continue;
      }

      const int pixelX = originX + (x + kQuietZone) * kScale;
      const int pixelY = originY + (y + kQuietZone) * kScale;
      M5.Display.fillRect(pixelX, pixelY, kScale, kScale, TFT_BLACK);
    }
  }
}

}  // namespace

void SetupRenderer::render(const char* apSsid, const IPAddress& ipAddress) {
  M5.Display.fillScreen(TFT_WHITE);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(20, 20);
  M5.Display.println("HomeDeck 配网");
  M5.Display.println();
  M5.Display.println("1. 连接开放热点");
  M5.Display.println(apSsid);
  M5.Display.println();
  M5.Display.println("2. 打开 192.168.4.1");
  M5.Display.print("当前热点 IP: ");
  M5.Display.println(ipAddress);

  drawQrCode(homedeck::buildWifiQrPayload(apSsid));
}
