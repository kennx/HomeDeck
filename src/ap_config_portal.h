#pragma once

#include <WebServer.h>

#include <functional>
#include <string>

#include "homedeck/config_types.h"

class ApConfigPortal {
 public:
  using SaveCallback = std::function<bool(const homedeck::SetupConfig& config)>;

  void begin(
      const char* apSsid,
      const homedeck::SetupConfig& defaults,
      SaveCallback onSave);
  void handleClient();

 private:
  void handleIndex();
  void handleSave();
  void sendPage(int statusCode, const homedeck::SetupConfig& values, const std::string& message);

  WebServer server_{80};
  std::string apSsid_;
  homedeck::SetupConfig defaults_{};
  SaveCallback onSave_;
  bool restartScheduled_ = false;
  unsigned long restartAtMs_ = 0;
};
