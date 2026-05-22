#pragma once

#include <string>

struct TimeSnapshot {
  std::string timeText;
  std::string dateText;
  bool timeValid = false;
  bool timeSynced = false;
};

class TimeService {
 public:
  bool begin(const char* timezonePosix, const char* ntpServer);
  TimeSnapshot snapshot() const;
  bool syncFromNtp();

 private:
  std::string timezonePosix_;
  std::string ntpServer_;
  bool rtcAvailable_ = false;
  bool timeSynced_ = false;
  time_t lastSyncedAt_ = 0;
};
