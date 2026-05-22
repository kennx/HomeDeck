#pragma once

#include <IPAddress.h>

class SetupRenderer {
 public:
  void render(const char* apSsid, const IPAddress& ipAddress);
};
