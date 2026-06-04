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
constexpr int kCalColCount = 7;

int cellLeftX(int col) {
  return kViewInsetX + col * kCalWidth / kCalColCount;
}

int cellRightX(int col) {
  return kViewInsetX + (col + 1) * kCalWidth / kCalColCount;
}

int cellCenterX(int col) {
  return (cellLeftX(col) + cellRightX(col)) / 2;
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
      daysInfo_[i].lunarMonth.clear();
      daysInfo_[i].lunarDay.clear();
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

          // 拆分农历日期为月份和日期（如 "三月十五" → "三月" + "十五"）
          // "月" 在 UTF-8 中为 3 字节（E6 9C 88），日期文本紧跟其后
          const auto monthPos = almanac.lunarDate.find("月");
          if (monthPos != std::string::npos && monthPos + 3 < almanac.lunarDate.size()) {
            daysInfo_[i].lunarMonth = almanac.lunarDate.substr(0, monthPos + 3);
            daysInfo_[i].lunarDay = almanac.lunarDate.substr(monthPos + 3);
          } else {
            daysInfo_[i].lunarMonth.clear();
            daysInfo_[i].lunarDay = almanac.lunarDate;
          }
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
      canvas.drawString(kWeeklyWeekdayLabels[col], cx, 60);
    }
    canvas.unloadFont();
  }

  // 7. 渲染 Row 2 (公历日期数字)
  if (canvas.loadFont(generated::kDeviceFontVlw)) {
    canvas.setTextDatum(textdatum_t::middle_center);
    for (int col = 0; col < 7; ++col) {
      const int cx = cellCenterX(col);
      const int cy = 110;

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
    constexpr int kLunarDateBottomY = 168;
    constexpr int kLunarRowHeight = 18;
    for (int col = 0; col < 7; ++col) {
      const int cx = cellCenterX(col);

      if (!daysInfo_[col].lunarFestival.empty()) {
        canvas.setTextDatum(textdatum_t::bottom_center);
        canvas.drawString(truncateUtf8(daysInfo_[col].lunarFestival, 4).c_str(), cx, kLunarDateBottomY);
      } else if (!daysInfo_[col].solarTerm.empty()) {
        canvas.setTextDatum(textdatum_t::bottom_center);
        canvas.drawString(truncateUtf8(daysInfo_[col].solarTerm, 4).c_str(), cx, kLunarDateBottomY);
      } else {
        // 农历日期：月份变化时显示两行（上月份下日期），日期统一靠底对齐
        bool showMonth = false;
        if (col == 0) {
          showMonth = true;
        } else if (!daysInfo_[col].lunarMonth.empty() &&
                   daysInfo_[col].lunarMonth != daysInfo_[col - 1].lunarMonth) {
          showMonth = true;
        }

        canvas.setTextDatum(textdatum_t::bottom_center);
        if (showMonth && !daysInfo_[col].lunarMonth.empty()) {
          canvas.drawString(daysInfo_[col].lunarMonth.c_str(), cx, kLunarDateBottomY - kLunarRowHeight);
        }
        canvas.drawString(daysInfo_[col].lunarDay.c_str(), cx, kLunarDateBottomY);
      }
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
          int lineY = 192 + linesDrawn * 16;
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

void CalendarView::render() {
  std::time_t now = std::time(nullptr);
  std::tm buf{};
  std::tm* local = now > 0 ? localtime_r(&now, &buf) : nullptr;

  CalendarData data{};
  const EnvironmentReading reading = readSht40Environment();
  if (reading.ok) {
    data.temperatureAvailable = true;
    data.temperatureCelsius = reading.temperatureCelsius;
    data.humidityAvailable = true;
    data.humidityPercent = reading.humidityPercent;
  }
  data.bottomCenterMessage = formatCurrentTimeHHMM();

  if (local == nullptr) {
    render(data);
    return;
  }

  render(data);
}

void CalendarView::renderWithOffset(int weekOffset) {
  weekOffset_ = weekOffset;
  hasCachedDaysInfo_ = false;
  render();
}

void CalendarView::renderSleep() {
  CalendarData data{};
  data.temperatureAvailable = false;
  data.humidityAvailable = false;
  data.bottomCenterMessage = "--:--";

  render(data);
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
