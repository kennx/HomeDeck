#pragma once

#include <array>
#include <string_view>

namespace homedeck {

struct TimezoneOption {
  std::string_view iana;
  std::string_view posix;
};

inline constexpr std::array<TimezoneOption, 20> kTimezoneCatalog{{
    {"Etc/UTC", "UTC0"},
    {"Pacific/Honolulu", "HST10"},
    {"America/Los_Angeles", "PST8PDT,M3.2.0,M11.1.0"},
    {"America/Denver", "MST7MDT,M3.2.0,M11.1.0"},
    {"America/Chicago", "CST6CDT,M3.2.0,M11.1.0"},
    {"America/New_York", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Sao_Paulo", "BRT3"},
    {"Atlantic/Reykjavik", "GMT0"},
    {"Europe/London", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"Europe/Berlin", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Paris", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Helsinki", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Africa/Johannesburg", "SAST-2"},
    {"Asia/Dubai", "GST-4"},
    {"Asia/Kolkata", "IST-5:30"},
    {"Asia/Bangkok", "ICT-7"},
    {"Asia/Shanghai", "CST-8"},
    {"Asia/Singapore", "SGT-8"},
    {"Asia/Tokyo", "JST-9"},
    {"Australia/Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
}};

inline constexpr const TimezoneOption* findTimezoneByIana(std::string_view iana) {
  for (const auto& option : kTimezoneCatalog) {
    if (option.iana == iana) {
      return &option;
    }
  }

  return nullptr;
}

}  // namespace homedeck
