#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#if defined(ARDUINO)
#include <WString.h>
#endif

#if __has_include(<uICAL.h>)
#include <uICAL.h>
#define HOMEDECK_HAS_UICAL 1
#else
#define HOMEDECK_HAS_UICAL 0
#endif

#include "homedeck/calendar_types.h"
#include "homedeck/home_screen.h"

struct ParsedPersonalCalendar {
  std::array<homedeck::EventRow, 8> events{};
  std::uint32_t eventCount = 0;
};

struct ParsedHolidayCalendar {
  std::string holidayText;
};

struct CalendarFetchResult {
  bool ok = false;
  std::string body;
  int statusCode = 0;
  std::string errorMessage;
};

namespace homedeck::calendar_detail {

struct RawCalendarEvent {
  std::string dtstart;
  std::string summary;
  bool isDateOnly = false;
};

inline bool isLeapYear(int year) {
  return (year % 400 == 0) || ((year % 4 == 0) && (year % 100 != 0));
}

inline int daysInMonth(int year, int month) {
  static constexpr int kDaysPerMonth[] = {
      31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
  };

   if (month < 1 || month > 12) {
    return -1;
  }

  if (month == 2 && isLeapYear(year)) {
    return 29;
  }

  return kDaysPerMonth[month - 1];
}

inline bool isValidCalendarDate(const homedeck::CalendarDate& date) {
  if (date.month < 1 || date.month > 12) {
    return false;
  }

  const int maxDay = daysInMonth(date.year, date.month);
  return maxDay > 0 && date.day >= 1 && date.day <= maxDay;
}

inline homedeck::CalendarDate nextDate(const homedeck::CalendarDate& date) {
  homedeck::CalendarDate result = date;
  ++result.day;
  if (result.day <= daysInMonth(result.year, result.month)) {
    return result;
  }

  result.day = 1;
  ++result.month;
  if (result.month <= 12) {
    return result;
  }

  result.month = 1;
  ++result.year;
  return result;
}

inline std::string pad2(int value) {
  if (value < 10) {
    return "0" + std::to_string(value);
  }

  return std::to_string(value);
}

inline std::string buildDateStamp(const homedeck::CalendarDate& date) {
  return std::to_string(date.year) + pad2(date.month) + pad2(date.day);
}

inline std::string buildDateTimeStamp(const homedeck::CalendarDate& date) {
  return buildDateStamp(date) + "T000000";
}

#if HOMEDECK_HAS_UICAL
inline uICAL::string toUicalString(const std::string& value) {
  return uICAL::string(value.c_str());
}

inline uICAL::DateTime makeUicalDateTime(const homedeck::CalendarDate& date) {
  return uICAL::DateTime(toUicalString(buildDateTimeStamp(date)));
}
#endif

inline std::string formatTimeText(std::string_view value) {
  const std::size_t tPos = value.find('T');
  if (tPos == std::string_view::npos || tPos + 5 >= value.size()) {
    return "";
  }

  return std::string(value.substr(tPos + 1, 2)) + ":" +
         std::string(value.substr(tPos + 3, 2));
}

inline void appendPersonalEvent(
    ParsedPersonalCalendar& result,
    std::string_view dtstart,
    std::string_view summary) {
  if (result.eventCount >= result.events.size()) {
    return;
  }

  auto& row = result.events[result.eventCount++];
  row.timeText = formatTimeText(dtstart);
  row.titleText = std::string(summary);
}

inline bool hasHolidaySummary(std::string_view holidayText) {
  return holidayText.size() > sizeof("节假日 ") - 1;
}

inline std::string trimLine(std::string_view line) {
  std::size_t begin = 0;
  while (begin < line.size() && (line[begin] == '\r' || line[begin] == '\n')) {
    ++begin;
  }

  std::size_t end = line.size();
  while (end > begin && (line[end - 1] == '\r' || line[end - 1] == '\n')) {
    --end;
  }

  return std::string(line.substr(begin, end - begin));
}

inline bool isFoldedLine(std::string_view line) {
  return !line.empty() && (line.front() == ' ' || line.front() == '\t');
}

inline std::string unfoldContinuation(std::string_view line) {
  std::string value = trimLine(line);
  if (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
    value.erase(value.begin());
  }
  return value;
}

template <typename EventConsumer>
inline void scanRawEvents(const char* ics, EventConsumer&& onEvent) {
  if (ics == nullptr) {
    return;
  }

  std::stringstream stream(ics);
  std::string line;
  bool inEvent = false;
  RawCalendarEvent current;
  std::string lastField;

  while (std::getline(stream, line)) {
    if (isFoldedLine(line) && inEvent) {
      const std::string continuation = unfoldContinuation(line);
      if (lastField == "DTSTART") {
        current.dtstart += continuation;
      } else if (lastField == "SUMMARY") {
        current.summary += continuation;
      }
      continue;
    }

    const std::string trimmed = trimLine(line);
    if (trimmed == "BEGIN:VEVENT") {
      inEvent = true;
      current = RawCalendarEvent{};
      lastField.clear();
      continue;
    }

    if (trimmed == "END:VEVENT") {
      if (inEvent) {
        onEvent(current);
      }
      inEvent = false;
      lastField.clear();
      continue;
    }

    if (!inEvent) {
      continue;
    }

    if (trimmed.rfind("DTSTART", 0) == 0) {
      const std::size_t colonPos = trimmed.find(':');
      if (colonPos != std::string::npos) {
        current.dtstart = trimmed.substr(colonPos + 1);
        current.isDateOnly = trimmed.find("VALUE=DATE") != std::string::npos;
        lastField = "DTSTART";
      }
      continue;
    }

    if (trimmed.rfind("SUMMARY", 0) == 0) {
      const std::size_t colonPos = trimmed.find(':');
      if (colonPos != std::string::npos) {
        current.summary = trimmed.substr(colonPos + 1);
        lastField = "SUMMARY";
      }
    }
  }
}

inline ParsedPersonalCalendar parsePersonalCalendarFallback(
    const char* ics,
    const homedeck::CalendarDate& targetDate) {
  ParsedPersonalCalendar result;
  const std::string targetStamp = buildDateStamp(targetDate);

  scanRawEvents(ics, [&](const RawCalendarEvent& event) {
    if (event.dtstart.size() < 8) {
      return;
    }

    if (event.dtstart.compare(0, 8, targetStamp) == 0) {
      appendPersonalEvent(result, event.dtstart, event.summary);
    }
  });

  return result;
}

inline ParsedHolidayCalendar parseHolidayCalendarFallback(
    const char* ics,
    const homedeck::CalendarDate& targetDate) {
  ParsedHolidayCalendar result;
  const std::string targetStamp = buildDateStamp(targetDate);

  scanRawEvents(ics, [&](const RawCalendarEvent& event) {
    if (!result.holidayText.empty()) {
      return;
    }

    if (event.dtstart.size() < 8) {
      return;
    }

    if (event.dtstart.compare(0, 8, targetStamp) == 0) {
      result.holidayText = "节假日 " + event.summary;
    }
  });

  return result;
}

#if HOMEDECK_HAS_UICAL
inline ParsedPersonalCalendar parsePersonalCalendarWithUical(
    const char* ics,
    const homedeck::CalendarDate& targetDate) {
  ParsedPersonalCalendar result;

  if (ics == nullptr) {
    return result;
  }

  const std::string icsText = ics;

#if defined(ARDUINO)
  String input(icsText.c_str());
  uICAL::istream_String stream(input);
#else
  std::stringstream input(icsText);
  uICAL::istream_stl stream(input);
#endif

  const auto calendar = uICAL::Calendar::load(stream);
  const auto endDate = nextDate(targetDate);
  const auto iterator = uICAL::new_ptr<uICAL::CalendarIter>(
      calendar,
      makeUicalDateTime(targetDate),
      makeUicalDateTime(endDate));

  while (iterator->next()) {
    const auto entry = iterator->current();
    const std::string start = entry->start().as_str().c_str();
    if (start.compare(0, 8, buildDateStamp(targetDate)) != 0) {
      continue;
    }

    appendPersonalEvent(result, start, entry->summary().c_str());
  }

  return result;
}

inline ParsedHolidayCalendar parseHolidayCalendarWithUical(
    const char* ics,
    const homedeck::CalendarDate& targetDate) {
  ParsedHolidayCalendar result;

  if (ics == nullptr) {
    return result;
  }

  const std::string icsText = ics;

#if defined(ARDUINO)
  String input(icsText.c_str());
  uICAL::istream_String stream(input);
#else
  std::stringstream input(icsText);
  uICAL::istream_stl stream(input);
#endif

  const auto calendar = uICAL::Calendar::load(stream);
  const auto endDate = nextDate(targetDate);
  const auto iterator = uICAL::new_ptr<uICAL::CalendarIter>(
      calendar,
      makeUicalDateTime(targetDate),
      makeUicalDateTime(endDate));

  while (iterator->next()) {
    const auto entry = iterator->current();
    const std::string start = entry->start().as_str().c_str();
    if (start.compare(0, 8, buildDateStamp(targetDate)) != 0) {
      continue;
    }

    const std::string summary = entry->summary().c_str();
    if (summary.empty()) {
      continue;
    }

    result.holidayText = std::string("节假日 ") + summary;
    return result;
  }

  return result;
}
#endif

}  // namespace homedeck::calendar_detail

class CalendarService {
 public:
  using Fetcher = std::function<CalendarFetchResult(const std::string& url)>;

  explicit CalendarService(Fetcher fetcher = Fetcher{}) : fetcher_(std::move(fetcher)) {
  }

  inline ParsedPersonalCalendar parsePersonalCalendarForDay(
      const char* ics,
      int year,
      int month,
      int day) const {
    const homedeck::CalendarDate targetDate{year, month, day};

    if (!homedeck::calendar_detail::isValidCalendarDate(targetDate)) {
      return {};
    }

#if HOMEDECK_HAS_UICAL
    try {
      const ParsedPersonalCalendar result =
          homedeck::calendar_detail::parsePersonalCalendarWithUical(
              ics, targetDate);
      if (result.eventCount > 0) {
        return result;
      }
    } catch (...) {
    }
#endif

    return homedeck::calendar_detail::parsePersonalCalendarFallback(
        ics, targetDate);
  }

  inline ParsedHolidayCalendar parseHolidayCalendarForDay(
      const char* ics,
      int year,
      int month,
      int day) const {
    const homedeck::CalendarDate targetDate{year, month, day};

    if (!homedeck::calendar_detail::isValidCalendarDate(targetDate)) {
      return {};
    }

#if HOMEDECK_HAS_UICAL
    try {
      const ParsedHolidayCalendar result =
          homedeck::calendar_detail::parseHolidayCalendarWithUical(
              ics, targetDate);
      if (homedeck::calendar_detail::hasHolidaySummary(result.holidayText)) {
        return result;
      }
    } catch (...) {
    }
#endif

    return homedeck::calendar_detail::parseHolidayCalendarFallback(
        ics, targetDate);
  }

  CalendarFetchResult fetchText(const std::string& url) const;
  std::string encodePersonalCalendarCache(const ParsedPersonalCalendar& calendar) const;
  ParsedPersonalCalendar decodePersonalCalendarCache(const std::string& payload) const;
  std::string encodeHolidayCalendarCache(const ParsedHolidayCalendar& calendar) const;
  ParsedHolidayCalendar decodeHolidayCalendarCache(const std::string& payload) const;

 private:
  Fetcher fetcher_;
};
