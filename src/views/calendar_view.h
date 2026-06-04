#pragma once

#include <ctime>
#include <string>

namespace homedeck {

struct CalendarData {
  bool temperatureAvailable = false;
  float temperatureCelsius = 0.0f;
  bool humidityAvailable = false;
  float humidityPercent = 0.0f;
  std::string bottomCenterMessage;
};

class CalendarView {
 public:
  struct WeeklyDayInfo {
    std::tm tmVal;
    std::string lunarDate;
    std::string lunarFestival;
    std::string solarTerm;
    std::string webcalFestival;
    bool isToday = false;
  };

  void render();
  void render(const CalendarData& data);
  void renderWithOffset(int weekOffset);
  void renderSleep();
  void reset();      // reset offset and render
  void resetOffset(); // reset offset only, no render

 private:
  int weekOffset_ = 0;
  WeeklyDayInfo daysInfo_[7];
  bool hasCachedDaysInfo_ = false;
  int cachedWeekOffset_ = -9999;
  int cachedTodayMday_ = -1;
  int cachedTodayMon_ = -1;
  int cachedTodayYear_ = -1;
};

}  // namespace homedeck
