#include "config/config_validator.h"

#include <cstdio>

#include "providers/timezone_catalog.h"

namespace homedeck {
namespace {

bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int daysInMonth(int year, int month) {
  switch (month) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
      return 31;
    case 4:
    case 6:
    case 9:
    case 11:
      return 30;
    case 2:
      return isLeapYear(year) ? 29 : 28;
    default:
      return 0;
  }
}

bool isBlank(std::string_view value) {
  return trim(value).empty();
}

ConfigValidationResult makeError(ConfigValidationError error, const char* message) {
  return ConfigValidationResult{error, message};
}

}  // namespace

std::string_view trim(std::string_view value) {
  std::size_t start = 0;
  while (start < value.size() && (value[start] == ' ' || value[start] == '\t' || value[start] == '\n' || value[start] == '\r')) {
    ++start;
  }
  std::size_t end = value.size();
  while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\n' || value[end - 1] == '\r')) {
    --end;
  }
  return value.substr(start, end - start);
}

bool parseLatitude(std::string_view value, double* out) {
  const std::string_view trimmed = trim(value);
  if (trimmed.empty()) return true;
  char* end = nullptr;
  const double d = std::strtod(std::string(trimmed).c_str(), &end);
  if (*end != '\0') return false;
  if (!(d >= -90.0 && d <= 90.0)) return false;
  if (out) *out = d;
  return true;
}

bool parseLongitude(std::string_view value, double* out) {
  const std::string_view trimmed = trim(value);
  if (trimmed.empty()) return true;
  char* end = nullptr;
  const double d = std::strtod(std::string(trimmed).c_str(), &end);
  if (*end != '\0') return false;
  if (!(d >= -180.0 && d <= 180.0)) return false;
  if (out) *out = d;
  return true;
}

bool parseManualDateTime(std::string_view value, ManualDateTime* out) {
  if (out == nullptr) {
    return false;
  }

  *out = ManualDateTime{};
  if (value.empty()) {
    return true;
  }

  ManualDateTime parsed{};
  parsed.present = true;
  int pos = 0;
  const int matched = std::sscanf(
      std::string(value).c_str(),
      "%d-%d-%dT%d:%d%n",
      &parsed.year,
      &parsed.month,
      &parsed.day,
      &parsed.hour,
      &parsed.minute,
      &pos);

  if (matched != 5 || pos != static_cast<int>(value.size()) ||
      !isManualDateTimeValid(parsed)) {
    return false;
  }

  *out = parsed;
  return true;
}

bool isManualDateTimeValid(const ManualDateTime& value) {
  if (!value.present) {
    return true;
  }
  if (value.year < 2024 || value.year > 2099) {
    return false;
  }
  if (value.month < 1 || value.month > 12) {
    return false;
  }
  if (value.day < 1 || value.day > daysInMonth(value.year, value.month)) {
    return false;
  }
  if (value.hour < 0 || value.hour > 23) {
    return false;
  }
  if (value.minute < 0 || value.minute > 59) {
    return false;
  }
  return value.second >= 0 && value.second <= 59;
}

ConfigValidationResult validateSetupSubmission(
    const SetupConfig& config,
    const ManualDateTime& manualDateTime) {
  if (findTimezoneByIana(config.timezoneIana) == nullptr) {
    return makeError(ConfigValidationError::InvalidTimezone, "时区不受支持。");
  }

  if (!isManualDateTimeValid(manualDateTime)) {
    return makeError(ConfigValidationError::InvalidManualDateTime, "手动时间格式无效。");
  }

  const bool hasWifi = !isBlank(config.wifiSsid);
  if (!hasWifi && config.autoRtcCorrection) {
    return makeError(ConfigValidationError::MissingManualDateTime,
                     "离线配置不能开启自动纠正。");
  }

  if (!hasWifi && !manualDateTime.present) {
    return makeError(ConfigValidationError::MissingManualDateTime, "离线配置必须填写手动时间。");
  }

  if (hasWifi && config.autoRtcCorrection && isBlank(config.ntpServer)) {
    return makeError(ConfigValidationError::MissingNtpServer, "自动纠正 RTC 时必须填写 NTP 服务器。");
  }

  if (hasWifi && !config.autoRtcCorrection && !manualDateTime.present) {
    return makeError(ConfigValidationError::MissingManualDateTime, "关闭自动纠正时必须填写手动时间。");
  }

  if (!parseLatitude(config.latitude, nullptr)) {
    return makeError(ConfigValidationError::InvalidLatitude, "纬度格式无效（范围：-90 ~ 90）。");
  }
  if (!parseLongitude(config.longitude, nullptr)) {
    return makeError(ConfigValidationError::InvalidLongitude, "经度格式无效（范围：-180 ~ 180）。");
  }

  return ConfigValidationResult{};
}

}  // namespace homedeck
