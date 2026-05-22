#include "home_renderer.h"

#include <M5Unified.h>

#include <algorithm>
#include <cstdint>
#include <string>

namespace {

constexpr int kMarginX = 20;
constexpr int kTimeY = 24;
constexpr int kDateY = 106;
constexpr int kLunarY = 144;
constexpr int kSolarTermY = 174;
constexpr int kHolidayY = 204;
constexpr int kMetricBoxY = 244;
constexpr int kMetricBoxWidth = 170;
constexpr int kMetricBoxHeight = 92;
constexpr int kHumidityBoxX = 210;
constexpr int kEventsTitleY = 362;
constexpr int kEventListY = 396;
constexpr int kEventRowHeight = 32;
constexpr int kEventTimeWidth = 92;
constexpr int kEventFooterY = 528;
constexpr int kStatusY = 566;

void trimLastUtf8CodePoint(std::string* text) {
  if (text == nullptr || text->empty()) {
    return;
  }

  std::size_t index = text->size();
  do {
    --index;
  } while (index > 0 && ((*text)[index] & 0xC0) == 0x80);

  text->erase(index);
}

std::string fitText(int size, int maxWidth, const std::string& text) {
  M5.Display.setTextSize(size);
  if (maxWidth <= 0 || text.empty()) {
    return "";
  }

  if (M5.Display.textWidth(text.c_str()) <= maxWidth) {
    return text;
  }

  std::string fitted = text;
  constexpr const char* kEllipsis = "...";
  const int ellipsisWidth = M5.Display.textWidth(kEllipsis);
  if (ellipsisWidth > maxWidth) {
    while (!fitted.empty() && M5.Display.textWidth(fitted.c_str()) > maxWidth) {
      trimLastUtf8CodePoint(&fitted);
    }
    return fitted;
  }

  while (!fitted.empty()) {
    trimLastUtf8CodePoint(&fitted);
    const std::string candidate = fitted + kEllipsis;
    if (M5.Display.textWidth(candidate.c_str()) <= maxWidth) {
      return candidate;
    }
  }

  return kEllipsis;
}

void drawText(int x, int y, int size, const std::string& text, int maxWidth) {
  M5.Display.setTextSize(size);
  M5.Display.setCursor(x, y);
  M5.Display.print(fitText(size, maxWidth, text).c_str());
}

void drawMetricBox(int x, int y, const char* label, const std::string& value) {
  M5.Display.drawRect(x, y, kMetricBoxWidth, kMetricBoxHeight, TFT_BLACK);

  drawText(x + 12, y + 12, 2, label, kMetricBoxWidth - 24);
  drawText(x + 12, y + 44, 4, value, kMetricBoxWidth - 24);
}

void drawEventRow(int x, int y, const homedeck::EventRow& row) {
  if (row.timeText.empty()) {
    drawText(x, y, 2, row.titleText, M5.Display.width() - x - kMarginX);
    return;
  }

  drawText(x, y, 2, row.timeText, kEventTimeWidth - 8);
  drawText(
      x + kEventTimeWidth,
      y,
      2,
      row.titleText,
      M5.Display.width() - (x + kEventTimeWidth) - kMarginX);
}

}  // namespace

void HomeRenderer::render(const homedeck::HomeViewModel& model) {
  M5.Display.setRotation(3);
  M5.Display.fillScreen(TFT_WHITE);
  M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
  M5.Display.setTextWrap(false);

  const int pageWidth = M5.Display.width();
  const int contentWidth = pageWidth - kMarginX * 2;

  drawText(kMarginX, kTimeY, 7, model.timeText.empty() ? "--:--" : model.timeText, contentWidth);
  drawText(kMarginX, kDateY, 2, model.dateText, contentWidth);
  drawText(kMarginX, kLunarY, 2, model.lunarText, contentWidth);
  drawText(kMarginX, kSolarTermY, 2, model.solarTermText, contentWidth);
  drawText(kMarginX, kHolidayY, 2, model.holidayText, contentWidth);

  drawMetricBox(kMarginX, kMetricBoxY, "温度", model.temperatureText);
  drawMetricBox(kHumidityBoxX, kMetricBoxY, "湿度", model.humidityText);

  drawText(kMarginX, kEventsTitleY, 2, "今日日程", contentWidth);

  const std::uint32_t visibleCount = std::min<std::uint32_t>(
      model.eventCount,
      static_cast<std::uint32_t>(model.eventRows.size()));
  if (visibleCount == 0) {
    if (!model.eventRows[0].titleText.empty()) {
      drawEventRow(kMarginX, kEventListY, model.eventRows[0]);
    }
  } else {
    for (std::uint32_t index = 0; index < visibleCount; ++index) {
      drawEventRow(
          kMarginX,
          kEventListY + static_cast<int>(index) * kEventRowHeight,
          model.eventRows[index]);
    }
  }

  if (model.eventCount > visibleCount) {
    drawText(
        kMarginX,
        kEventFooterY,
        2,
        "还有 " + std::to_string(model.eventCount - visibleCount) + " 项",
        contentWidth);
  }

  drawText(kMarginX, kStatusY, 2, homedeck::buildStatusText(model), contentWidth);
}
