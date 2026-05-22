#include "calendar_service.h"

#include <sstream>

#if defined(ARDUINO)
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#endif

namespace {

constexpr char kFieldSeparator = '\t';

std::string trimTrailingCr(std::string value) {
  while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) {
    value.pop_back();
  }
  return value;
}

std::string sanitizeCacheField(const std::string& value) {
  std::string sanitized = value;
  for (char& ch : sanitized) {
    if (ch == '\r' || ch == '\n' || ch == kFieldSeparator) {
      ch = ' ';
    }
  }
  return sanitized;
}

CalendarFetchResult fetchOverHttp(const std::string& url) {
#if defined(ARDUINO)
  HTTPClient http;

  if (url.rfind("https://", 0) == 0) {
    WiFiClientSecure client;
    client.setInsecure();
    if (!http.begin(client, url.c_str())) {
      return {false, "", 0, "begin https failed"};
    }
  } else if (url.rfind("http://", 0) == 0) {
    if (!http.begin(url.c_str())) {
      return {false, "", 0, "begin http failed"};
    }
  } else {
    return {false, "", 0, "unsupported url scheme"};
  }

  const int statusCode = http.GET();
  if (statusCode <= 0) {
    const String error = http.errorToString(statusCode);
    http.end();
    return {false, "", statusCode, error.c_str()};
  }

  const String body = http.getString();
  http.end();
  return {statusCode >= 200 && statusCode < 300, body.c_str(), statusCode, ""};
#else
  (void)url;
  return {false, "", 0, "network fetch unavailable"};
#endif
}

}  // namespace

CalendarFetchResult CalendarService::fetchText(const std::string& url) const {
  if (fetcher_) {
    return fetcher_(url);
  }

  return fetchOverHttp(url);
}

std::string CalendarService::encodePersonalCalendarCache(
    const ParsedPersonalCalendar& calendar) const {
  std::ostringstream stream;
  stream << calendar.eventCount << '\n';

  const std::uint32_t limit = std::min<std::uint32_t>(
      calendar.eventCount, static_cast<std::uint32_t>(calendar.events.size()));
  for (std::uint32_t index = 0; index < limit; ++index) {
    stream << sanitizeCacheField(calendar.events[index].timeText) << kFieldSeparator
           << sanitizeCacheField(calendar.events[index].titleText) << '\n';
  }

  return stream.str();
}

ParsedPersonalCalendar CalendarService::decodePersonalCalendarCache(
    const std::string& payload) const {
  ParsedPersonalCalendar result;
  if (payload.empty()) {
    return result;
  }

  std::stringstream stream(payload);
  std::string line;
  if (!std::getline(stream, line)) {
    return result;
  }

  const std::uint32_t storedCount = static_cast<std::uint32_t>(std::strtoul(line.c_str(), nullptr, 10));
  while (result.eventCount < result.events.size() && std::getline(stream, line)) {
    const std::string trimmed = trimTrailingCr(line);
    const std::size_t split = trimmed.find(kFieldSeparator);
    if (split == std::string::npos) {
      continue;
    }

    result.events[result.eventCount].timeText = trimmed.substr(0, split);
    result.events[result.eventCount].titleText = trimmed.substr(split + 1);
    ++result.eventCount;
  }

  result.eventCount = std::min<std::uint32_t>(storedCount, result.eventCount);

  return result;
}

std::string CalendarService::encodeHolidayCalendarCache(
    const ParsedHolidayCalendar& calendar) const {
  return sanitizeCacheField(calendar.holidayText);
}

ParsedHolidayCalendar CalendarService::decodeHolidayCalendarCache(
    const std::string& payload) const {
  return ParsedHolidayCalendar{trimTrailingCr(payload)};
}
