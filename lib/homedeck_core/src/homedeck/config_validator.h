#pragma once

#include <cctype>
#include <string>
#include <string_view>

#include "homedeck/config_types.h"
#include "homedeck/timezone_catalog.h"

namespace homedeck {

inline bool isBlank(std::string_view value) {
  for (const unsigned char ch : value) {
    if (!std::isspace(ch)) {
      return false;
    }
  }

  return true;
}

inline bool hasSupportedCalendarUrlScheme(std::string_view value) {
  return value.substr(0, sizeof("webcal://") - 1) == "webcal://" ||
         value.substr(0, sizeof("http://") - 1) == "http://" ||
         value.substr(0, sizeof("https://") - 1) == "https://";
}

inline std::string normalizeCalendarUrl(std::string_view value) {
  if (value.substr(0, sizeof("webcal://") - 1) == "webcal://") {
    std::string normalized{"https://"};
    normalized.append(value.substr(sizeof("webcal://") - 1));
    return normalized;
  }

  return std::string{value};
}

inline ValidationError validateSetupConfig(const SetupConfig& config) {
  if (isBlank(config.wifiSsid)) {
    return ValidationError::MissingWifiSsid;
  }

  if (findTimezoneByIana(config.timezoneIana) == nullptr) {
    return ValidationError::InvalidTimezone;
  }

  if (isBlank(config.ntpServer)) {
    return ValidationError::MissingNtpServer;
  }

  if (!config.personalCalendarUrl.empty() &&
      !hasSupportedCalendarUrlScheme(config.personalCalendarUrl)) {
    return ValidationError::InvalidPersonalCalendarUrl;
  }

  if (!config.holidayCalendarUrl.empty() &&
      !hasSupportedCalendarUrlScheme(config.holidayCalendarUrl)) {
    return ValidationError::InvalidHolidayCalendarUrl;
  }

  return ValidationError::None;
}

inline const char* validationMessage(ValidationError error) {
  switch (error) {
    case ValidationError::MissingWifiSsid:
      return "Wi-Fi 名称不能为空。";
    case ValidationError::InvalidTimezone:
      return "时区必须是支持列表中的 IANA 时区。";
    case ValidationError::MissingNtpServer:
      return "NTP 服务器不能为空。";
    case ValidationError::InvalidPersonalCalendarUrl:
      return "个人日程地址必须以 webcal://、http:// 或 https:// 开头。";
    case ValidationError::InvalidHolidayCalendarUrl:
      return "节假日地址必须以 webcal://、http:// 或 https:// 开头。";
    case ValidationError::None:
      return "";
  }

  return "";
}

}  // namespace homedeck
