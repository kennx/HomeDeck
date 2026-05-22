#pragma once

#include <string>

namespace homedeck {

struct SetupConfig {
  std::string wifiSsid;
  std::string wifiPassword;
  std::string timezoneIana;
  std::string ntpServer;
  std::string personalCalendarUrl;
  std::string holidayCalendarUrl;
};

enum class ValidationError {
  None,
  MissingWifiSsid,
  InvalidTimezone,
  MissingNtpServer,
  InvalidPersonalCalendarUrl,
  InvalidHolidayCalendarUrl,
};

}  // namespace homedeck
