#pragma once

#include <cstdint>
#include <string>

constexpr int ECC_LOW = 0;

struct QRCode {
  int size = 21;
};

inline std::string gLastQrCodeText;

inline int qrcode_getBufferSize(int) {
  return 64;
}

inline void qrcode_initText(QRCode* qrcode, std::uint8_t*, int, int, const char* text) {
  gLastQrCodeText = text != nullptr ? text : "";
  if (qrcode != nullptr) {
    qrcode->size = 21;
  }
}

inline bool qrcode_getModule(const QRCode*, int x, int y) {
  return ((x + y) % 3) == 0;
}
