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

std::string buildSetupPageHtml(
    const std::string& apSsid,
    const SetupConfig& values,
    const std::vector<WifiNetwork>& networks,
    const std::string& message,
    const std::string& batteryInfo) {
  // 1. 提前拼接 Wi-Fi 网格列表
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

  // 2. 提前拼接时区下拉框列表
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

  // 3. 一次扫描流式替换
  std::ostringstream out;
  std::string_view temp(SETUP_PAGE_TEMPLATE);
  size_t lastPos = 0;
  size_t pos = 0;

  while ((pos = temp.find("{{", lastPos)) != std::string_view::npos) {
    // 写入占位符之前的片段
    out << temp.substr(lastPos, pos - lastPos);

    size_t endPos = temp.find("}}", pos);
    if (endPos == std::string_view::npos) {
      out << temp.substr(pos);
      return out.str();
    }

    std::string_view placeholder = temp.substr(pos, endPos + 2 - pos);
    if (placeholder == "{{AP_SSID}}") {
      out << htmlEscape(apSsid);
    } else if (placeholder == "{{BATTERY_INFO_STYLE}}") {
      out << (batteryInfo.empty() ? "display:none;" : "display:flex;");
    } else if (placeholder == "{{BATTERY_INFO}}") {
      out << htmlEscape(batteryInfo);
    } else if (placeholder == "{{WIFI_GRID_ITEMS}}") {
      out << wifiGrid.str();
    } else if (placeholder == "{{ERROR_CONTAINER_STYLE}}") {
      out << (message.empty() ? "display:none;" : "display:flex;");
    } else if (placeholder == "{{ERROR_MESSAGE}}") {
      out << htmlEscape(message);
    } else if (placeholder == "{{WIFI_SSID}}") {
      out << htmlEscape(values.wifiSsid);
    } else if (placeholder == "{{WIFI_PASSWORD}}") {
      out << htmlEscape(values.wifiPassword);
    } else if (placeholder == "{{NTP_SERVER}}") {
      out << htmlEscape(values.ntpServer);
    } else if (placeholder == "{{LATITUDE}}") {
      out << htmlEscape(values.latitude);
    } else if (placeholder == "{{LONGITUDE}}") {
      out << htmlEscape(values.longitude);
    } else if (placeholder == "{{AUTO_RTC_CHECKED}}") {
      out << (values.autoRtcCorrection ? "checked" : "");
    } else if (placeholder == "{{AUTO_RTC_DISABLED}}") {
      out << (values.wifiSsid.empty() ? "disabled" : "");
    } else if (placeholder == "{{TIMEZONE_OPTION_ITEMS}}") {
      out << tzOptions.str();
    } else {
      // 无法识别的占位符，原样输出
      out << placeholder;
    }

    lastPos = endPos + 2;
  }
  
  // 写入剩余片段
  out << temp.substr(lastPos);
  return out.str();
}

}  // namespace homedeck
