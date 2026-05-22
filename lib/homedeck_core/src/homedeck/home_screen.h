#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace homedeck {

struct EventRow {
  std::string timeText;
  std::string titleText;
};

struct VisibleEvents {
  std::array<EventRow, 4> visible{};
  std::uint32_t visibleCount = 0;
  std::string footerText;
};

struct HomeViewModel {
  std::string timeText;
  std::string dateText;
  std::string lunarText;
  std::string solarTermText;
  std::string holidayText;
  std::string temperatureText;
  std::string humidityText;
  std::array<EventRow, 4> eventRows{};
  std::uint32_t eventCount = 0;
  bool wifiConnected = false;
  bool timeSynced = false;
  bool calendarFresh = false;
  bool sensorAvailable = true;
};

inline std::string buildStatusText(const HomeViewModel& model) {
  if (!model.wifiConnected) {
    return "离线";
  }

  if (!model.timeSynced) {
    return "时间未同步";
  }

  if (!model.calendarFresh) {
    return "日历未更新";
  }

  if (!model.sensorAvailable) {
    return "传感器不可用";
  }

  return "已更新";
}

inline VisibleEvents clampTodayEvents(
    const std::array<EventRow, 5>& source,
    std::uint32_t sourceCount,
    std::uint32_t maxVisible) {
  VisibleEvents result;

  std::uint32_t copyableCount = sourceCount;
  if (copyableCount > source.size()) {
    copyableCount = static_cast<std::uint32_t>(source.size());
  }

  std::uint32_t visibleLimit = maxVisible;
  if (visibleLimit > result.visible.size()) {
    visibleLimit = static_cast<std::uint32_t>(result.visible.size());
  }

  result.visibleCount = copyableCount;
  if (result.visibleCount > visibleLimit) {
    result.visibleCount = visibleLimit;
  }

  for (std::uint32_t index = 0; index < result.visibleCount; ++index) {
    result.visible[index] = source[index];
  }

  if (sourceCount > result.visibleCount) {
    result.footerText = "还有 " +
                        std::to_string(sourceCount - result.visibleCount) +
                        " 项";
  }

  return result;
}

inline std::string makeEmptyEventMessage(bool configured, bool hasCache) {
  if (!configured) {
    return "未配置个人日程";
  }

  if (!hasCache) {
    return "今日日程不可用";
  }

  return "今日日程为空";
}

inline std::string makeHolidayText(bool configured, const std::string& value) {
  if (!configured) {
    return "未配置节假日";
  }

  if (value.empty()) {
    return "节假日 无";
  }

  return value;
}

}  // namespace homedeck
