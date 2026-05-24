#pragma once

#include <string>

namespace homedeck {

class HomeRenderer {
 public:
  void render();
  void renderConfigPortal(const std::string& apSsid, const std::string& ipAddress);

 private:
  void drawQrCode(const std::string& text, int left, int top, int size);
};

}  // namespace homedeck
