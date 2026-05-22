#pragma once

#include <cstdint>
#include <string>

#include "../lib/homedeck_core/src/homedeck/lunar_data.h"

struct LunarDayInfo {
  std::string lunarText;
  std::string solarTermText;
};

namespace lunar_calendar_detail {

struct LunarDate {
  int year = 0;
  int month = 0;
  int day = 0;
  bool isLeap = false;
};

inline std::int64_t daysFromCivil(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yearOfEra = static_cast<unsigned>(year - era * 400);
  const unsigned dayOfYear =
      (153 * (month + (month > 2 ? static_cast<unsigned>(-3) : 9)) + 2) / 5 +
      day - 1;
  const unsigned dayOfEra =
      yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
  return static_cast<std::int64_t>(era) * 146097 +
         static_cast<std::int64_t>(dayOfEra) - 719468;
}

inline bool isSupportedSolarDate(int year, int month, int day) {
  const std::int64_t current = daysFromCivil(
      year, static_cast<unsigned>(month), static_cast<unsigned>(day));
  const std::int64_t lowerBound = daysFromCivil(1900, 1U, 31U);
  const std::int64_t upperBound = daysFromCivil(2100, 12U, 31U);
  return current >= lowerBound && current <= upperBound;
}

inline bool isSolarLeapYear(int year) {
  return ((year % 4) == 0 && (year % 100) != 0) || (year % 400) == 0;
}

inline int solarDaysInMonth(int year, int month) {
  static constexpr int kSolarMonthDays[] = {31, 28, 31, 30, 31, 30,
                                            31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) {
    return -1;
  }
  if (month == 2) {
    return isSolarLeapYear(year) ? 29 : 28;
  }
  return kSolarMonthDays[month - 1];
}

inline std::uint32_t lunarInfoForYear(int year) {
  return homedeck::lunar_data::kLunarInfo[year - homedeck::lunar_data::kStartYear];
}

inline int leapMonth(int year) {
  return static_cast<int>(lunarInfoForYear(year) & 0x0f);
}

inline int leapDays(int year) {
  if (leapMonth(year) == 0) {
    return 0;
  }
  return (lunarInfoForYear(year) & 0x10000U) != 0U ? 30 : 29;
}

inline int monthDays(int year, int month) {
  return (lunarInfoForYear(year) & (0x10000U >> month)) != 0U ? 30 : 29;
}

inline int lunarYearDays(int year) {
  int sum = 348;
  for (std::uint32_t mask = 0x8000; mask > 0x8; mask >>= 1) {
    if ((lunarInfoForYear(year) & mask) != 0U) {
      ++sum;
    }
  }
  return sum + leapDays(year);
}

inline LunarDate solarToLunar(int year, int month, int day) {
  const std::int64_t offset =
      daysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day)) -
      daysFromCivil(1900, 1U, 31U);

  LunarDate result;
  std::int64_t remaining = offset;
  result.year = homedeck::lunar_data::kStartYear;

  while (result.year < homedeck::lunar_data::kEndYear) {
    const int daysInYear = lunarYearDays(result.year);
    if (remaining < daysInYear) {
      break;
    }
    remaining -= daysInYear;
    ++result.year;
  }

  const int leap = leapMonth(result.year);
  result.month = 1;

  while (result.month <= 12) {
    const int daysInMonth = result.isLeap ? leapDays(result.year)
                                          : monthDays(result.year, result.month);
    if (remaining < daysInMonth) {
      break;
    }

    remaining -= daysInMonth;
    if (!result.isLeap && leap == result.month) {
      result.isLeap = true;
    } else {
      if (result.isLeap) {
        result.isLeap = false;
      }
      ++result.month;
    }
  }

  result.day = static_cast<int>(remaining) + 1;
  return result;
}

inline int solarTermDay(int year, int termIndex) {
  const std::string encoded =
      homedeck::lunar_data::kTermInfo[year - homedeck::lunar_data::kStartYear];
  const int chunkIndex = (termIndex - 1) / 4;
  const int positionInChunk = (termIndex - 1) % 4;
  std::string decimal = std::to_string(
      std::stoul(encoded.substr(static_cast<std::size_t>(chunkIndex) * 5U, 5U),
                 nullptr,
                 16));
  while (decimal.size() < 6U) {
    decimal.insert(decimal.begin(), '0');
  }

  switch (positionInChunk) {
    case 0:
      return std::stoi(decimal.substr(0, 1));
    case 1:
      return std::stoi(decimal.substr(1, 2));
    case 2:
      return std::stoi(decimal.substr(3, 1));
    default:
      return std::stoi(decimal.substr(4, 2));
  }
}

inline std::string toChinaMonth(int month) {
  return std::string(homedeck::lunar_data::kMonthTexts[month - 1]) + "月";
}

inline std::string toChinaDay(int day) {
  if (day == 10) {
    return "初十";
  }
  if (day == 20) {
    return "二十";
  }
  if (day == 30) {
    return "三十";
  }

  return std::string(homedeck::lunar_data::kDayPrefixes[day / 10]) +
         homedeck::lunar_data::kNumberTexts[day % 10];
}

inline std::string describeSolarTerm(int year, int month, int day) {
  const int firstTermIndex = month * 2 - 1;
  const int secondTermIndex = month * 2;

  if (solarTermDay(year, firstTermIndex) == day) {
    return std::string("节气 ") +
           homedeck::lunar_data::kSolarTerms[firstTermIndex - 1];
  }
  if (solarTermDay(year, secondTermIndex) == day) {
    return std::string("节气 ") +
           homedeck::lunar_data::kSolarTerms[secondTermIndex - 1];
  }
  return "节气 无";
}

}  // namespace lunar_calendar_detail

class LunarCalendarService {
 public:
  inline LunarDayInfo describeDate(int year, int month, int day) const {
    LunarDayInfo result{"农历 无", "节气 无"};

    if (year < homedeck::lunar_data::kStartYear ||
        year > homedeck::lunar_data::kEndYear) {
      return result;
    }

    if (month < 1 || month > 12) {
      return result;
    }

    const int daysInMonth = lunar_calendar_detail::solarDaysInMonth(year, month);
    if (day < 1 || day > daysInMonth) {
      return result;
    }

    if (!lunar_calendar_detail::isSupportedSolarDate(year, month, day)) {
      return result;
    }

    const lunar_calendar_detail::LunarDate lunar =
        lunar_calendar_detail::solarToLunar(year, month, day);
    result.lunarText = "农历 ";
    if (lunar.isLeap) {
      result.lunarText += "闰";
    }
    result.lunarText += lunar_calendar_detail::toChinaMonth(lunar.month);
    result.lunarText += lunar_calendar_detail::toChinaDay(lunar.day);
    result.solarTermText =
        lunar_calendar_detail::describeSolarTerm(year, month, day);
    return result;
  }
};
