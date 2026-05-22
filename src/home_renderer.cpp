#include "home_renderer.h"

#include <M5Unified.h>

#include <algorithm>
#include <cstdint>
#include <string>

namespace {

enum class RenderFont {
  kDefault,
  kChinese,
};

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

std::string fitText(M5Canvas& canvas, int size, int maxWidth, const std::string& text) {
  canvas.setTextSize(size);
  if (maxWidth <= 0 || text.empty()) {
    return "";
  }

  if (canvas.textWidth(text.c_str()) <= maxWidth) {
    return text;
  }

  std::string fitted = text;
  constexpr const char* kEllipsis = "...";
  const int ellipsisWidth = canvas.textWidth(kEllipsis);
  if (ellipsisWidth > maxWidth) {
    while (!fitted.empty() && canvas.textWidth(fitted.c_str()) > maxWidth) {
      trimLastUtf8CodePoint(&fitted);
    }
    return fitted;
  }

  while (!fitted.empty()) {
    trimLastUtf8CodePoint(&fitted);
    const std::string candidate = fitted + kEllipsis;
    if (canvas.textWidth(candidate.c_str()) <= maxWidth) {
      return candidate;
    }
  }

  return kEllipsis;
}

void setRenderFont(M5Canvas& canvas, RenderFont font) {
  canvas.setFont(font == RenderFont::kChinese ? &fonts::efontCN_14 : nullptr);
}

std::size_t findFirstNonAsciiByte(const std::string& text) {
  for (std::size_t index = 0; index < text.size(); ++index) {
    if ((static_cast<unsigned char>(text[index]) & 0x80) != 0) {
      return index;
    }
  }
  return std::string::npos;
}

void drawText(
    M5Canvas& canvas,
    int x,
    int y,
    int size,
    const std::string& text,
    int maxWidth,
    RenderFont font) {
  setRenderFont(canvas, font);
  canvas.setTextSize(size);
  canvas.setCursor(x, y);
  canvas.print(fitText(canvas, size, maxWidth, text).c_str());
}

void drawMetricBox(M5Canvas& canvas, int x, int y, const char* label, const std::string& value) {
  canvas.drawRect(x, y, kMetricBoxWidth, kMetricBoxHeight, TFT_BLACK);

  drawText(canvas, x + 12, y + 12, 2, label, kMetricBoxWidth - 24, RenderFont::kChinese);
  drawText(canvas, x + 12, y + 44, 4, value, kMetricBoxWidth - 24, RenderFont::kDefault);
}

void drawTemperatureMetricBox(M5Canvas& canvas, int x, int y, const char* label, const std::string& value) {
  canvas.drawRect(x, y, kMetricBoxWidth, kMetricBoxHeight, TFT_BLACK);

  drawText(canvas, x + 12, y + 12, 2, label, kMetricBoxWidth - 24, RenderFont::kChinese);

  const int valueX = x + 12;
  const int valueY = y + 44;
  const int valueWidth = kMetricBoxWidth - 24;
  const std::size_t suffixIndex = findFirstNonAsciiByte(value);
  if (suffixIndex == std::string::npos) {
    drawText(canvas, valueX, valueY, 4, value, valueWidth, RenderFont::kDefault);
    return;
  }

  const std::string prefix = value.substr(0, suffixIndex);
  const std::string suffix = value.substr(suffixIndex);

  setRenderFont(canvas, RenderFont::kChinese);
  const std::string fittedSuffix = fitText(canvas, 1, valueWidth, suffix);
  const int suffixWidth = canvas.textWidth(fittedSuffix.c_str());

  setRenderFont(canvas, RenderFont::kDefault);
  const std::string fittedPrefix = fitText(canvas, 4, std::max(0, valueWidth - suffixWidth), prefix);
  canvas.setTextSize(4);
  canvas.setCursor(valueX, valueY);
  canvas.print(fittedPrefix.c_str());
  const int prefixWidth = canvas.textWidth(fittedPrefix.c_str());

  setRenderFont(canvas, RenderFont::kChinese);
  canvas.setTextSize(1);
  canvas.setCursor(valueX + prefixWidth, valueY + 16);
  canvas.print(fittedSuffix.c_str());
}

void drawEventRow(M5Canvas& canvas, int x, int y, const homedeck::EventRow& row) {
  if (row.timeText.empty()) {
    drawText(
        canvas,
        x,
        y,
        2,
        row.titleText,
        canvas.width() - x - kMarginX,
        RenderFont::kChinese);
    return;
  }

  drawText(canvas, x, y, 2, row.timeText, kEventTimeWidth - 8, RenderFont::kDefault);
  drawText(
      canvas,
      x + kEventTimeWidth,
      y,
      2,
      row.titleText,
      canvas.width() - (x + kEventTimeWidth) - kMarginX,
      RenderFont::kChinese);
}

}  // namespace

void HomeRenderer::render(const homedeck::HomeViewModel& model) {
  M5Canvas canvas(&M5.Display);
  canvas.createSprite(M5.Display.width(), M5.Display.height());
  canvas.fillSprite(TFT_WHITE);
  canvas.setTextColor(TFT_BLACK, TFT_WHITE);
  canvas.setTextWrap(false);

  const int pageWidth = canvas.width();
  const int contentWidth = pageWidth - kMarginX * 2;

  drawText(
      canvas,
      kMarginX,
      kTimeY,
      7,
      model.timeText.empty() ? "--:--" : model.timeText,
      contentWidth,
      RenderFont::kDefault);
  drawText(canvas, kMarginX, kDateY, 2, model.dateText, contentWidth, RenderFont::kChinese);
  drawText(canvas, kMarginX, kLunarY, 2, model.lunarText, contentWidth, RenderFont::kChinese);
  drawText(canvas, kMarginX, kSolarTermY, 2, model.solarTermText, contentWidth, RenderFont::kChinese);
  drawText(canvas, kMarginX, kHolidayY, 2, model.holidayText, contentWidth, RenderFont::kChinese);

  drawTemperatureMetricBox(canvas, kMarginX, kMetricBoxY, "温度", model.temperatureText);
  drawMetricBox(canvas, kHumidityBoxX, kMetricBoxY, "湿度", model.humidityText);

  drawText(canvas, kMarginX, kEventsTitleY, 2, "今日日程", contentWidth, RenderFont::kChinese);

  const std::uint32_t visibleCount = std::min<std::uint32_t>(
      model.eventCount,
      static_cast<std::uint32_t>(model.eventRows.size()));
  if (visibleCount == 0) {
    if (!model.eventRows[0].titleText.empty()) {
      drawEventRow(canvas, kMarginX, kEventListY, model.eventRows[0]);
    }
  } else {
    for (std::uint32_t index = 0; index < visibleCount; ++index) {
      drawEventRow(
          canvas,
          kMarginX,
          kEventListY + static_cast<int>(index) * kEventRowHeight,
          model.eventRows[index]);
    }
  }

  if (model.eventCount > visibleCount) {
    drawText(
        canvas,
        kMarginX,
        kEventFooterY,
        2,
        "还有 " + std::to_string(model.eventCount - visibleCount) + " 项",
        contentWidth,
        RenderFont::kChinese);
  }

  drawText(
      canvas,
      kMarginX,
      kStatusY,
      2,
      homedeck::buildStatusText(model),
      contentWidth,
      RenderFont::kChinese);

  M5.Display.startWrite();
  canvas.pushSprite(0, 0);
  M5.Display.endWrite();
  M5.Display.waitDisplay();
  canvas.deleteSprite();
}
