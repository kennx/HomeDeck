#include "config/setup_page.h"

#include <algorithm>
#include <sstream>

#include "providers/timezone_catalog.h"
#include "generated/setup_page_html.h" // 包含由 tools/sync_preview.py 自动生成的原始 HTML 模板

namespace homedeck {

std::vector<WifiNetwork> selectTopWifiNetworks(
    const std::vector<WifiNetwork>& networks,
    std::size_t limit) {
  std::vector<WifiNetwork> selected = networks;
  std::sort(selected.begin(), selected.end(), [](const WifiNetwork& left, const WifiNetwork& right) {
    return left.rssi > right.rssi;
  });
  if (selected.size() > limit) {
    selected.resize(limit);
  }
  return selected;
}

std::string htmlEscape(const std::string& value) {
  std::string escaped;
  for (const char ch : value) {
    switch (ch) {
      case '&':
        escaped += "&amp;";
        break;
      case '<':
        escaped += "&lt;";
        break;
      case '>':
        escaped += "&gt;";
        break;
      case '"':
        escaped += "&quot;";
        break;
      case '\'':
        escaped += "&#39;";
        break;
      default:
        escaped += ch;
        break;
    }
  }
  return escaped;
}

// 辅助函数：全局字符串占位符替换
static std::string replaceAll(std::string str, const std::string& from, const std::string& to) {
  size_t startPos = 0;
  while ((startPos = str.find(from, startPos)) != std::string::npos) {
    str.replace(startPos, from.length(), to);
    startPos += to.length();
  }
  return str;
}

std::string buildSetupPageHtml(
    const std::string& apSsid,
    const SetupConfig& values,
    const std::vector<WifiNetwork>& networks,
    const std::string& message,
    const std::string& batteryInfo) {
  std::string html(SETUP_PAGE_TEMPLATE);

  // 1. 替换热点名 AP_SSID
  html = replaceAll(html, "{{AP_SSID}}", htmlEscape(apSsid));

  // 1.5 替换电池信息显示样式和内容
  if (!batteryInfo.empty()) {
    html = replaceAll(html, "{{BATTERY_INFO_STYLE}}", "display:flex;");
    html = replaceAll(html, "{{BATTERY_INFO}}", htmlEscape(batteryInfo));
  } else {
    html = replaceAll(html, "{{BATTERY_INFO_STYLE}}", "display:none;");
    html = replaceAll(html, "{{BATTERY_INFO}}", "");
  }

  // 2. 替换动态的 Wi-Fi 网格列表
  std::ostringstream wifiGrid;
  for (const auto& network : networks) {
    std::string signalQuality = "good";
    if (network.rssi < -80) signalQuality = "weak";
    else if (network.rssi < -65) signalQuality = "mid";

    wifiGrid << "<button type=\"button\" class=\"wifi-pill\" data-ssid=\"" << htmlEscape(network.ssid) << "\" data-sig=\"" << signalQuality << "\">";
    wifiGrid << "<span>" << htmlEscape(network.ssid) << "</span>";
    wifiGrid << "<div class=\"wifi-meta\">";
    wifiGrid << "<div class=\"sig-bars\"><span class=\"sig-bar\"></span><span class=\"sig-bar\"></span><span class=\"sig-bar\"></span><span class=\"sig-bar\"></span></div>";
    wifiGrid << network.rssi << " dBm</div></button>";
  }
  html = replaceAll(html, "{{WIFI_GRID_ITEMS}}", wifiGrid.str());

  // 3. 替换错误提示的显示样式和信息内容
  if (!message.empty()) {
    html = replaceAll(html, "{{ERROR_CONTAINER_STYLE}}", "display:flex;");
    html = replaceAll(html, "{{ERROR_MESSAGE}}", htmlEscape(message));
  } else {
    html = replaceAll(html, "{{ERROR_CONTAINER_STYLE}}", "display:none;");
    html = replaceAll(html, "{{ERROR_MESSAGE}}", "");
  }

  // 4. 替换表单控件的初始输入值
  html = replaceAll(html, "{{WIFI_SSID}}", htmlEscape(values.wifiSsid));
  html = replaceAll(html, "{{WIFI_PASSWORD}}", htmlEscape(values.wifiPassword));
  html = replaceAll(html, "{{NTP_SERVER}}", htmlEscape(values.ntpServer));
  html = replaceAll(html, "{{LATITUDE}}", htmlEscape(values.latitude));
  html = replaceAll(html, "{{LONGITUDE}}", htmlEscape(values.longitude));

  // 5. 替换自动 RTC 校时的勾选和启用状态
  html = replaceAll(html, "{{AUTO_RTC_CHECKED}}", values.autoRtcCorrection ? "checked" : "");
  html = replaceAll(html, "{{AUTO_RTC_DISABLED}}", values.wifiSsid.empty() ? "disabled" : "");

  // 6. 替换动态的时区下拉框 options 列表
  std::ostringstream tzOptions;
  std::size_t timezoneCount = 0;
  const auto* timezones = timezoneCatalog(&timezoneCount);
  for (std::size_t i = 0; i < timezoneCount; ++i) {
    tzOptions << "<option value=\"" << timezones[i].iana << "\"";
    if (values.timezoneIana == timezones[i].iana) {
      tzOptions << " selected";
    }
    tzOptions << ">" << timezones[i].label << "</option>";
  }
  html = replaceAll(html, "{{TIMEZONE_OPTION_ITEMS}}", tzOptions.str());

  return html;
}

}  // namespace homedeck
