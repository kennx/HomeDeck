#pragma once

#include <string>

namespace homedeck {

class ConfigPortalRenderer {
 public:
  void renderConfigPortal(const std::string& apSsid, const std::string& ipAddress);
};

}  // namespace homedeck
