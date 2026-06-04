#pragma once

#include <ctime>
#include <string>

namespace homedeck {

struct CalendarData {
  int year = 0;
  int month = 0;        // 1-12
  int day = 0;          // 当天，用于高亮
  int todayWeekday = 0; // 0=周日
  bool temperatureAvailable = false;
  float temperatureCelsius = 0.0f;
  bool humidityAvailable = false;
  float humidityPercent = 0.0f;
  std::string bottomCenterMessage;
  std::string lunarDate;
  std::string solarTerm;
  std::string festival;
  int nextSpecialMonth = 0;
  int nextSpecialDay = 0;
  std::string nextSpecialTerm;
  std::string nextSpecialFestival;
  int secondSpecialMonth = 0;
  int secondSpecialDay = 0;
  std::string secondSpecialTerm;
  std::string secondSpecialFestival;
  int todayMonth = 0;
  int todayDay = 0;
};

CalendarData makeCalendarData(const std::tm& localTime);
CalendarData makeCurrentCalendarData();
void applySht40ToCalendar(CalendarData& data);

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
  void onButtonA();  // prev week
  void onButtonB();  // next week
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
