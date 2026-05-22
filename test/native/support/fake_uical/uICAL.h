#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace uICAL {

class string {
 public:
  string() = default;

  explicit string(const char* value) : value_(value == nullptr ? "" : value) {
  }

  const char* c_str() const {
    return value_.c_str();
  }

 private:
  std::string value_;
};

class DateTime {
 public:
  DateTime() = default;

  explicit DateTime(const string& value) : value_(value.c_str()) {
  }

  string as_str() const {
    return string(value_.c_str());
  }

 private:
  std::string value_;
};

class istream_stl {
 public:
  explicit istream_stl(std::stringstream& stream) {
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    text_ = buffer.str();
  }

  const std::string& text() const {
    return text_;
  }

 private:
  std::string text_;
};

class Event {
 public:
  Event(std::string start, std::string summary)
      : start_(string(start.c_str())), summary_(std::move(summary)) {
  }

  DateTime start() const {
    return start_;
  }

  string summary() const {
    return string(summary_.c_str());
  }

 private:
  DateTime start_;
  std::string summary_;
};

class Calendar {
 public:
  static std::shared_ptr<Calendar> load(istream_stl& stream) {
    auto calendar = std::make_shared<Calendar>();
    calendar->parse(stream.text());
    return calendar;
  }

  const std::vector<Event>& events() const {
    return events_;
  }

 private:
  struct RawEvent {
    std::string dtstart;
    std::string summary;
    bool skip = false;
  };

  void parse(const std::string& ics) {
    std::stringstream stream(ics);
    std::string line;
    bool inEvent = false;
    RawEvent current;

    while (std::getline(stream, line)) {
      while (!line.empty() &&
             (line.back() == '\r' || line.back() == '\n')) {
        line.pop_back();
      }

      if (line == "BEGIN:VEVENT") {
        inEvent = true;
        current = RawEvent{};
        continue;
      }

      if (line == "END:VEVENT") {
        if (inEvent && !current.skip && !current.dtstart.empty()) {
          events_.emplace_back(current.dtstart, current.summary);
        }
        inEvent = false;
        continue;
      }

      if (!inEvent) {
        continue;
      }

      if (line.rfind("DTSTART", 0) == 0) {
        const std::size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
          current.dtstart = line.substr(colonPos + 1);
        }
        continue;
      }

      if (line.rfind("SUMMARY:", 0) == 0) {
        current.summary = line.substr(8);
        continue;
      }

      if (line == "X-HOMEDECK-FAKE-UICAL-SKIP:1") {
        current.skip = true;
      }
    }
  }

  std::vector<Event> events_;
};

class CalendarIter {
 public:
  CalendarIter(
      std::shared_ptr<Calendar> calendar,
      const DateTime&,
      const DateTime&)
      : calendar_(std::move(calendar)) {
  }

  bool next() {
    if (calendar_ == nullptr || index_ >= calendar_->events().size()) {
      current_ = nullptr;
      return false;
    }

    current_ = &calendar_->events()[index_++];
    return true;
  }

  const Event* current() const {
    return current_;
  }

 private:
  std::shared_ptr<Calendar> calendar_;
  std::size_t index_ = 0;
  const Event* current_ = nullptr;
};

template <typename T, typename... Args>
std::shared_ptr<T> new_ptr(Args&&... args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

}  // namespace uICAL
