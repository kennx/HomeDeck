#pragma once

#if __cplusplus < 201703L
#include <cstddef>
#include <cstring>
#endif

#include <string>

#if __cplusplus < 201703L
namespace std {
class string_view {
 public:
  constexpr string_view() : data_(""), size_(0) {}

  template <size_t N>
  constexpr string_view(const char (&value)[N]) : data_(value), size_(N - 1) {}

  string_view(const char* value) : data_(value), size_(value ? std::strlen(value) : 0) {}

  string_view(const std::string& value) : data_(value.data()), size_(value.size()) {}

  constexpr const char* begin() const { return data_; }
  constexpr const char* end() const { return data_ + size_; }
  constexpr const char* data() const { return data_; }
  constexpr size_t size() const { return size_; }

 private:
  const char* data_;
  size_t size_;
};

inline bool operator==(string_view lhs, string_view rhs) {
  return lhs.size() == rhs.size() && std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
}
}  // namespace std
#endif

#include "homedeck/config_types.h"
#include "homedeck/timezone_catalog.h"

namespace homedeck {

inline std::string escapeHtml(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());

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

inline std::string buildWifiQrPayload(const std::string& apSsid) {
  return "WIFI:T:nopass;S:" + apSsid + ";;";
}

inline std::string buildSetupPageHtml(
    const std::string& apSsid,
    const SetupConfig& defaults,
    const std::string& message) {
  const std::string escapedApSsid = escapeHtml(apSsid);
  const std::string wifiSsid = escapeHtml(defaults.wifiSsid);
  const std::string wifiPassword = escapeHtml(defaults.wifiPassword);
  const std::string timezone = escapeHtml(defaults.timezoneIana);
  const std::string ntpServer = defaults.ntpServer.empty()
      ? "pool.ntp.org"
      : escapeHtml(defaults.ntpServer);
  const std::string personalCalendarUrl = escapeHtml(defaults.personalCalendarUrl);
  const std::string holidayCalendarUrl = escapeHtml(defaults.holidayCalendarUrl);

  std::string timezoneOptions;
  for (const auto& option : kTimezoneCatalog) {
    timezoneOptions += "<option value=\"";
    timezoneOptions += escapeHtml(std::string(option.iana));
    timezoneOptions += "\"></option>";
  }

  std::string html;
  html.reserve(4600);
  html += "<!doctype html><html lang=\"zh-CN\"><head><meta charset=\"utf-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<title>HomeDeck 配网</title>";
  html += "<style>body{font-family:Arial,sans-serif;max-width:720px;margin:0 auto;padding:24px;line-height:1.5;color:#111;background:#f5f5f5;}";
  html += "main{background:#fff;border-radius:12px;padding:24px;box-shadow:0 2px 12px rgba(0,0,0,.08);}label{display:block;font-weight:600;margin-top:16px;}";
  html += "input{width:100%;padding:12px;margin-top:8px;box-sizing:border-box;}button{margin-top:24px;padding:12px 18px;font-size:16px;}";
  html += ".message{margin-top:16px;padding:12px;border-radius:8px;background:#eef6ff;}code{background:#f0f0f0;padding:2px 6px;border-radius:4px;}</style></head><body><main>";
  html += "<h1>HomeDeck 配网</h1>";
  html += "<p>请先连接开放 Wi-Fi 热点 <code>";
  html += escapedApSsid;
  html += "</code>，然后填写家庭网络和日历设置。</p>";

  if (!message.empty()) {
    html += "<div class=\"message\">";
    html += escapeHtml(message);
    html += "</div>";
  }

  html += "<form method=\"post\" action=\"/save\">";
  html += "<label for=\"wifi_ssid\">Wi-Fi 名称</label>";
  html += "<input id=\"wifi_ssid\" name=\"wifi_ssid\" value=\"";
  html += wifiSsid;
  html += "\" autocomplete=\"ssid\">";
  html += "<label for=\"wifi_password\">Wi-Fi 密码</label>";
  html += "<input id=\"wifi_password\" name=\"wifi_password\" type=\"password\" value=\"";
  html += wifiPassword;
  html += "\" autocomplete=\"current-password\">";
  html += "<label for=\"timezone\">时区</label>";
  html += "<input id=\"timezone\" name=\"timezone\" list=\"timezone-options\" value=\"";
  html += timezone;
  html += "\">";
  html += "<datalist id=\"timezone-options\">";
  html += timezoneOptions;
  html += "</datalist>";
  html += "<label for=\"ntp_server\">NTP 服务器</label>";
  html += "<input id=\"ntp_server\" name=\"ntp_server\" value=\"";
  html += ntpServer;
  html += "\" placeholder=\"pool.ntp.org\">";
  html += "<label for=\"personal_calendar_url\">个人日程 webcal 地址</label>";
  html += "<input id=\"personal_calendar_url\" name=\"personal_calendar_url\" value=\"";
  html += personalCalendarUrl;
  html += "\" placeholder=\"webcal://example.com/personal.ics\">";
  html += "<label for=\"holiday_calendar_url\">节假日 webcal 地址</label>";
  html += "<input id=\"holiday_calendar_url\" name=\"holiday_calendar_url\" value=\"";
  html += holidayCalendarUrl;
  html += "\" placeholder=\"webcal://example.com/holiday.ics\">";
  html += "<button type=\"submit\">保存并重启</button></form></main></body></html>";

  return html;
}

}  // namespace homedeck
