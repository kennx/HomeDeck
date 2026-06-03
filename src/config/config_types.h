#pragma once

#include <cstdint>
#include <string>

namespace homedeck {

constexpr const char* kDefaultLatitude = "31.2304";
constexpr const char* kDefaultLongitude = "121.4737";

struct SetupConfig {
  std::string wifiSsid;
  std::string wifiPassword;
  std::string timezoneIana = "Asia/Shanghai";
  bool autoRtcCorrection = false;
  std::string ntpServer = "pool.ntp.org";
  std::string latitude;
  std::string longitude;
};

struct ManualDateTime {
  bool present = false;
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
};

struct WifiNetwork {
  std::string ssid;
  int32_t rssi = 0;
};

enum class ConfigValidationError {
  None,
  MissingManualDateTime,
  MissingNtpServer,
  InvalidManualDateTime,
  InvalidTimezone,
  InvalidLatitude,
  InvalidLongitude,
};

struct ConfigValidationResult {
  ConfigValidationError error = ConfigValidationError::None;
  const char* message = "";
  bool ok() const { return error == ConfigValidationError::None; }
};

}  // namespace homedeck
