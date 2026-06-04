#include "views/calendar_view.h"

#include <M5Unified.h>
#include <cstdio>
#include <ctime>
#include <string>
#include <map>

#include "views/almanac_view.h"
#include "providers/almanac_provider.h"
#include "providers/webcal_provider.h"
#include "generated/device_font_vlw.h"
#include "system/render_context.h"
#include "system/sht40_reader.h"
#include "views/view_common.h"

namespace homedeck {
namespace {

constexpr int kCalWidth = 376;
constexpr int kCalHeaderHeight = 27;
constexpr int kCalWeekdayTopY = 51;
constexpr int kCalWeekdayHeight = 47;
constexpr int kCalDateStartY = 98;
constexpr int kCalDateRowHeight = 47;
constexpr int kCalDateRowGap = 0;
constexpr int kCalColCount = 7;
constexpr int kCalDateRows = 6;

const char* calendarWeekdayLabel(int index) {
  static constexpr const char* kLabels[] = {"日", "一", "二", "三", "四", "五", "六"};
  if (index < 0 || index >= 7) return "";
  return kLabels[index];
}

int daysInMonth(int year, int month) {
  static constexpr int kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) return 31;
  if (month == 2) {
    const bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    return isLeap ? 29 : 28;
  }
  return kDays[month - 1];
}

int cellLeftX(int col) {
  return kViewInsetX + col * kCalWidth / kCalColCount;
}

int cellRightX(int col) {
  return kViewInsetX + (col + 1) * kCalWidth / kCalColCount;
}

int cellCenterX(int col) {
  return (cellLeftX(col) + cellRightX(col)) / 2;
}

void normalizeYearMonth(int& year, int& month) {
  while (month > 12) {
    month -= 12;
    year++;
  }
  while (month < 1) {
    month += 12;
    year--;
  }
}

std::string truncateUtf8(const std::string& str, size_t maxCharCount) {
  size_t charCount = 0;
  size_t byteIdx = 0;
  while (byteIdx < str.size() && charCount < maxCharCount) {
    unsigned char c = str[byteIdx];
    if (c < 0x80) {
      byteIdx += 1;
    } else if ((c & 0xE0) == 0xC0) {
      byteIdx += 2;
    } else if ((c & 0xF0) == 0xE0) {
      byteIdx += 3;
    } else if ((c & 0xF8) == 0xF0) {
      byteIdx += 4;
    } else {
      byteIdx += 1;
    }
    charCount++;
  }
  return str.substr(0, byteIdx);
}

}  // namespace

void CalendarView::render(const CalendarData& data) {
  M5Canvas& canvas = sprite();
  prepareScreen(canvas);

  // 1. 获取基准时间
  std::time_t now = std::time(nullptr);
  std::tm local{};
  std::tm* pLocal = now > 0 ? localtime_r(&now, &local) : nullptr;
  if (pLocal == nullptr) {
    local = fallbackLocalTime();
    pLocal = &local;
  }

  bool canReuseCache = hasCachedDaysInfo_ &&
                       (cachedWeekOffset_ == weekOffset_) &&
                       (cachedTodayMday_ == pLocal->tm_mday) &&
                       (cachedTodayMon_ == pLocal->tm_mon) &&
                       (cachedTodayYear_ == pLocal->tm_year);

  if (!canReuseCache) {
    std::tm todayTm = *pLocal;
    todayTm.tm_hour = 12;
    todayTm.tm_min = 0;
    todayTm.tm_sec = 0;
    std::mktime(&todayTm);

    // 计算本周一的日期
    int daysToMonday = (todayTm.tm_wday == 0) ? 6 : (todayTm.tm_wday - 1);
    std::tm mondayTm = todayTm;
    mondayTm.tm_mday -= daysToMonday;
    mondayTm.tm_mday += weekOffset_ * 7;
    std::mktime(&mondayTm);

    // 2. 初始化这 7 天
    for (int i = 0; i < 7; ++i) {
      std::tm t = mondayTm;
      t.tm_mday += i;
      std::mktime(&t);
      daysInfo_[i].tmVal = t;
      daysInfo_[i].isToday = false;
      daysInfo_[i].lunarDate.clear();
      daysInfo_[i].lunarFestival.clear();
      daysInfo_[i].solarTerm.clear();
      daysInfo_[i].webcalFestival.clear();

      if (weekOffset_ == 0 &&
          t.tm_mday == pLocal->tm_mday &&
          t.tm_mon == pLocal->tm_mon &&
          t.tm_year == pLocal->tm_year) {
        daysInfo_[i].isToday = true;
      }
    }

    // 3. 加载 Webcal 缓存节日
    std::map<std::string, std::string> cachedFestivals;
    loadCachedFestivals(cachedFestivals);
    for (int i = 0; i < 7; ++i) {
      char keyBuf[32];
      std::snprintf(keyBuf, sizeof(keyBuf), "%04d-%02d-%02d",
                    daysInfo_[i].tmVal.tm_year + 1900,
                    daysInfo_[i].tmVal.tm_mon + 1,
                    daysInfo_[i].tmVal.tm_mday);
      auto it = cachedFestivals.find(keyBuf);
      if (it != cachedFestivals.end()) {
        daysInfo_[i].webcalFestival = it->second;
      }
    }

    // 4. 查询 Almanac 农历及农历节日等
    AlmanacProvider provider;
    AlmanacLookupDate lookupDates[7];
    for (int i = 0; i < 7; ++i) {
      lookupDates[i].year = daysInfo_[i].tmVal.tm_year + 1900;
      lookupDates[i].month = daysInfo_[i].tmVal.tm_mon + 1;
      lookupDates[i].day = daysInfo_[i].tmVal.tm_mday;
    }
    provider.lookupEach(lookupDates, 7, [&](const AlmanacLookupDate& date, const AlmanacDayData& almanac) {
      for (int i = 0; i < 7; ++i) {
        if (lookupDates[i].year == date.year &&
            lookupDates[i].month == date.month &&
            lookupDates[i].day == date.day) {
          daysInfo_[i].lunarDate = almanac.lunarDate;
          daysInfo_[i].lunarFestival = lookupLunarFestival(almanac.lunarDate);
          daysInfo_[i].solarTerm = almanac.solarTerm;
          break;
        }
      }
      return false;
    });

    hasCachedDaysInfo_ = true;
    cachedWeekOffset_ = weekOffset_;
    cachedTodayMday_ = pLocal->tm_mday;
    cachedTodayMon_ = pLocal->tm_mon;
    cachedTodayYear_ = pLocal->tm_year;
  }

  // 5. 渲染 Header
  if (canvas.loadFont(generated::kDeviceFontVlw)) {
    canvas.setTextColor(TFT_BLACK, TFT_WHITE);

    // Left: Year of target week
    canvas.setTextDatum(textdatum_t::top_left);
    canvas.drawString(formatYear(daysInfo_[0].tmVal.tm_year + 1900).c_str(), kViewInsetX, kViewHeaderTopY);

    // Center: Chinese month name(s) of target week
    std::string centerText;
    if (daysInfo_[0].tmVal.tm_mon == daysInfo_[6].tmVal.tm_mon) {
      centerText = chineseMonthName(daysInfo_[0].tmVal.tm_mon);
    } else {
      centerText = std::string(chineseMonthName(daysInfo_[0].tmVal.tm_mon)) + "/" + chineseMonthName(daysInfo_[6].tmVal.tm_mon);
    }
    canvas.setTextDatum(textdatum_t::top_center);
    canvas.drawString(centerText.c_str(), kViewCenterX, kViewHeaderTopY);

    // Right: "周视图"
    canvas.setTextDatum(textdatum_t::top_right);
    canvas.drawString("周视图", kViewRightX, kViewHeaderTopY);

    canvas.unloadFont();
  }

  // 6. 渲染 Row 1 (星期名: 一, 二, 三, 四, 五, 六, 日)
  if (canvas.loadFont(generated::kDeviceFontVlw)) {
    canvas.setTextColor(TFT_BLACK, TFT_WHITE);
    canvas.setTextDatum(textdatum_t::middle_center);
    static constexpr const char* kWeeklyWeekdayLabels[] = {"一", "二", "三", "四", "五", "六", "日"};
    for (int col = 0; col < 7; ++col) {
      const int cx = cellCenterX(col);
      canvas.drawString(kWeeklyWeekdayLabels[col], cx, 140);
    }
    canvas.unloadFont();
  }

  // 7. 渲染 Row 2 (公历日期数字)
  if (canvas.loadFont(generated::kDeviceFontVlw)) {
    canvas.setTextDatum(textdatum_t::middle_center);
    for (int col = 0; col < 7; ++col) {
      const int cx = cellCenterX(col);
      const int cy = 220;

      if (daysInfo_[col].isToday) {
        canvas.fillSmoothCircle(cx, cy, 20, TFT_BLACK);
        canvas.setTextColor(TFT_WHITE);
      } else {
        canvas.setTextColor(TFT_BLACK, TFT_WHITE);
      }
      canvas.drawString(std::to_string(daysInfo_[col].tmVal.tm_mday).c_str(), cx, cy);
    }
    canvas.unloadFont();
  }

  // 8. 渲染 Row 3 (农历日期/节日/节气)
  if (canvas.loadFont(generated::kDeviceLunarFontVlw)) {
    canvas.setTextColor(TFT_BLACK, TFT_WHITE);
    canvas.setTextDatum(textdatum_t::middle_center);
    for (int col = 0; col < 7; ++col) {
      const int cx = cellCenterX(col);

      std::string rawLunarText = daysInfo_[col].lunarFestival;
      if (rawLunarText.empty()) {
        rawLunarText = daysInfo_[col].solarTerm;
      }
      if (rawLunarText.empty()) {
        rawLunarText = daysInfo_[col].lunarDate;
      }

      std::string lunarText = truncateUtf8(rawLunarText, 4);
      canvas.drawString(lunarText.c_str(), cx, 300);
    }
    canvas.unloadFont();
  }

  // 9. 渲染 Row 4 (Webcal 节日)
  if (canvas.loadFont(generated::kDeviceLunarFontVlw)) {
    canvas.setTextColor(TFT_BLACK, TFT_WHITE);
    canvas.setTextDatum(textdatum_t::top_center);
    for (int col = 0; col < 7; ++col) {
      if (!daysInfo_[col].webcalFestival.empty()) {
        const int cx = cellCenterX(col);

        // 按空格分词
        std::vector<std::string> tokens;
        std::string token;
        for (char ch : daysInfo_[col].webcalFestival) {
          if (ch == ' ') {
            if (!token.empty()) {
              tokens.push_back(token);
              token.clear();
            }
          } else {
            token += ch;
          }
        }
        if (!token.empty()) {
          tokens.push_back(token);
        }

        int linesDrawn = 0;
        for (const auto& tok : tokens) {
          if (linesDrawn >= 3) break;
          int lineY = 350 + linesDrawn * 16;
          canvas.drawString(truncateUtf8(tok, 4).c_str(), cx, lineY);
          linesDrawn++;
        }
      }
    }
    canvas.unloadFont();
  }

  // 10. 渲染底部状态栏
  if (canvas.loadFont(generated::kDeviceFontVlw)) {
    canvas.setTextColor(TFT_BLACK, TFT_WHITE);
    drawBottomStatusBar(canvas, {data.temperatureAvailable, data.temperatureCelsius,
                                 data.humidityAvailable, data.humidityPercent,
                                 data.bottomCenterMessage});
    canvas.unloadFont();
  }

  pushScreen(canvas);
}

CalendarData makeCalendarData(const std::tm& localTime) {
  CalendarData data{};
  data.year = localTime.tm_year + 1900;
  data.month = localTime.tm_mon + 1;
  data.day = localTime.tm_mday;
  data.todayWeekday = localTime.tm_wday;
  data.todayMonth = data.month;
  data.todayDay = data.day;

  if (gAlmanacCache.year == data.year && gAlmanacCache.month == data.month &&
      gAlmanacCache.day == data.day && gAlmanacCache.calendar.valid) {
    data.lunarDate = gAlmanacCache.calendar.lunarDate;
    data.solarTerm = gAlmanacCache.calendar.solarTerm;
    data.festival = gAlmanacCache.calendar.festival;
    data.nextSpecialMonth = gAlmanacCache.calendar.nextSpecialMonth;
    data.nextSpecialDay = gAlmanacCache.calendar.nextSpecialDay;
    data.nextSpecialTerm = gAlmanacCache.calendar.nextSpecialTerm;
    data.nextSpecialFestival = gAlmanacCache.calendar.nextSpecialFestival;
    data.secondSpecialMonth = gAlmanacCache.calendar.secondSpecialMonth;
    data.secondSpecialDay = gAlmanacCache.calendar.secondSpecialDay;
    data.secondSpecialTerm = gAlmanacCache.calendar.secondSpecialTerm;
    data.secondSpecialFestival = gAlmanacCache.calendar.secondSpecialFestival;
    return data;
  }

  AlmanacProvider provider;
  std::vector<AlmanacLookupDate> dates;
  dates.reserve(36);
  dates.push_back({data.year, data.month, data.day});
  std::tm searchTm = localTime;
  for (int offset = 1; offset <= 35; ++offset) {
    searchTm.tm_mday += 1;
    searchTm.tm_hour = 12;
    std::mktime(&searchTm);
    dates.push_back({searchTm.tm_year + 1900, searchTm.tm_mon + 1, searchTm.tm_mday});
  }

  bool foundFirst = false;
  bool foundToday = false;
  const bool foundAny = provider.lookupEach(dates.data(), dates.size(), [&](const AlmanacLookupDate& date, const AlmanacDayData& almanac) {
    if (date.year == data.year && date.month == data.month && date.day == data.day) {
      foundToday = true;
      data.lunarDate = almanac.lunarDate;
      data.solarTerm = almanac.solarTerm;
      data.festival = lookupLunarFestival(almanac.lunarDate);
      writeHomeAlmanacCache(data.year, data.month, data.day, almanac);
      return false;
    }

    const std::string nextFestival = lookupLunarFestival(almanac.lunarDate);
    if (!almanac.solarTerm.empty() || !nextFestival.empty()) {
      if (!foundFirst) {
        data.nextSpecialMonth = date.month;
        data.nextSpecialDay = date.day;
        data.nextSpecialTerm = almanac.solarTerm;
        data.nextSpecialFestival = nextFestival;
        foundFirst = true;
      } else {
        data.secondSpecialMonth = date.month;
        data.secondSpecialDay = date.day;
        data.secondSpecialTerm = almanac.solarTerm;
        data.secondSpecialFestival = nextFestival;
        return true;
      }
    }
    return false;
  });

  if (!foundAny || !foundToday) {
    return data;
  }

  prepareAlmanacCacheDate(data.year, data.month, data.day);
  gAlmanacCache.calendar.valid = true;
  setAlmanacCacheString(gAlmanacCache.calendar.lunarDate, sizeof(gAlmanacCache.calendar.lunarDate), data.lunarDate);
  gAlmanacCache.calendar.solarTerm[0] = '\0';
  if (!data.solarTerm.empty()) {
    setAlmanacCacheString(gAlmanacCache.calendar.solarTerm, sizeof(gAlmanacCache.calendar.solarTerm), data.solarTerm);
  }
  setAlmanacCacheString(gAlmanacCache.calendar.festival, sizeof(gAlmanacCache.calendar.festival), data.festival);
  gAlmanacCache.calendar.nextSpecialMonth = data.nextSpecialMonth;
  gAlmanacCache.calendar.nextSpecialDay = data.nextSpecialDay;
  setAlmanacCacheString(gAlmanacCache.calendar.nextSpecialTerm, sizeof(gAlmanacCache.calendar.nextSpecialTerm), data.nextSpecialTerm);
  setAlmanacCacheString(
      gAlmanacCache.calendar.nextSpecialFestival,
      sizeof(gAlmanacCache.calendar.nextSpecialFestival),
      data.nextSpecialFestival);
  gAlmanacCache.calendar.secondSpecialMonth = data.secondSpecialMonth;
  gAlmanacCache.calendar.secondSpecialDay = data.secondSpecialDay;
  setAlmanacCacheString(
      gAlmanacCache.calendar.secondSpecialTerm,
      sizeof(gAlmanacCache.calendar.secondSpecialTerm),
      data.secondSpecialTerm);
  setAlmanacCacheString(
      gAlmanacCache.calendar.secondSpecialFestival,
      sizeof(gAlmanacCache.calendar.secondSpecialFestival),
      data.secondSpecialFestival);

  return data;
}

void applySht40ToCalendar(CalendarData& data) {
  const EnvironmentReading reading = readSht40Environment();
  if (reading.ok) {
    data.temperatureAvailable = true;
    data.temperatureCelsius = reading.temperatureCelsius;
    data.humidityAvailable = true;
    data.humidityPercent = reading.humidityPercent;
  }
}

CalendarData makeCurrentCalendarData() {
  const std::time_t now = std::time(nullptr);
  std::tm buf{};
  const std::tm* local = now > 0 ? localtime_r(&now, &buf) : nullptr;
  if (local == nullptr) {
    std::tm fallback = fallbackLocalTime();
    return makeCalendarData(fallback);
  }
  CalendarData data = makeCalendarData(*local);
  applySht40ToCalendar(data);
  return data;
}

void CalendarView::render() {
  std::time_t now = std::time(nullptr);
  std::tm buf{};
  std::tm* local = now > 0 ? localtime_r(&now, &buf) : nullptr;
  if (local == nullptr) {
    std::tm fallback = fallbackLocalTime();
    CalendarData data{};
    applySht40ToCalendar(data);
    data.bottomCenterMessage = formatCurrentTimeHHMM();

    render(data);
    return;
  }

  CalendarData data{};
  applySht40ToCalendar(data);
  data.bottomCenterMessage = formatCurrentTimeHHMM();

  render(data);
}

void CalendarView::renderWithOffset(int weekOffset) {
  weekOffset_ = weekOffset;
  render();
}

void CalendarView::renderSleep() {
  CalendarData data{};
  data.temperatureAvailable = false;
  data.humidityAvailable = false;
  data.bottomCenterMessage = "--:--";

  render(data);
}

void CalendarView::onButtonA() {
  if (weekOffset_ > -520) {
    weekOffset_--;
    renderWithOffset(weekOffset_);
  }
}

void CalendarView::onButtonB() {
  if (weekOffset_ < 520) {
    weekOffset_++;
    renderWithOffset(weekOffset_);
  }
}

void CalendarView::reset() {
  weekOffset_ = 0;
  render();
}

void CalendarView::resetOffset() {
  weekOffset_ = 0;
  hasCachedDaysInfo_ = false;
}

}  // namespace homedeck
