#include "config/setup_page.h"

#include <algorithm>
#include <sstream>

#include "providers/timezone_catalog.h"

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
    const std::string& message) {
  std::ostringstream html;
  html << "<!doctype html><html><head><meta charset=\"utf-8\">";
  html << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html << "<title>HomeDeck Setup</title>";
  html << "<style>";
  html << ":root {";
  html << "  --primary-color: #4f46e5;";
  html << "  --primary-color-rgb: 79, 70, 229;";
  html << "  --primary-hover: #4338ca;";
  html << "  --color-success: #10b981;";
  html << "  --color-warning: #F59E0B;";
  html << "  --color-error: #ef4444;";
  html << "  --bg-disabled: #E5E7EB;";
  html << "  --text-primary: #1f2937;";
  html << "  --text-secondary: #4b5563;";
  html << "  --text-muted: #9ca3af;";
  html << "  --bg-main: #f3f4f6;";
  html << "  --bg-card: #ffffff;";
  html << "  --border-color: #e5e7eb;";
  html << "}";
  html << "* { box-sizing: border-box; margin: 0; padding: 0; }";
  html << "body {";
  html << "  font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", Roboto, \"Helvetica Neue\", Arial, sans-serif;";
  html << "  background-color: var(--bg-main);";
  html << "  color: var(--text-primary);";
  html << "  line-height: 1.5;";
  html << "  padding: 40px 20px;";
  html << "  display: flex;";
  html << "  flex-direction: column;";
  html << "  align-items: center;";
  html << "}";
  html << ".container { width: 100%; max-width: 600px; }";
  html << ".banner {";
  html << "  background: linear-gradient(135deg, rgba(var(--primary-color-rgb), 1) 0%, #312e81 100%);";
  html << "  color: #ffffff;";
  html << "  padding: 24px;";
  html << "  border-radius: 16px;";
  html << "  margin-bottom: 24px;";
  html << "  box-shadow: 0 10px 15px -3px rgba(var(--primary-color-rgb), 0.3);";
  html << "  display: flex;";
  html << "  align-items: center;";
  html << "  gap: 16px;";
  html << "  position: relative;";
  html << "  overflow: hidden;";
  html << "}";
  html << ".banner::after {";
  html << "  content: '';";
  html << "  position: absolute;";
  html << "  top: -50%;";
  html << "  right: -30%;";
  html << "  width: 250px;";
  html << "  height: 250px;";
  html << "  background: radial-gradient(circle, rgba(255,255,255,0.15) 0%, rgba(255,255,255,0) 70%);";
  html << "  border-radius: 50%;";
  html << "}";
  html << ".banner svg { width: 40px; height: 40px; flex-shrink: 0; stroke: currentColor; }";
  html << ".banner-info { flex-grow: 1; z-index: 1; }";
  html << ".banner-title { font-size: 22px; font-weight: 700; margin-bottom: 6px; letter-spacing: -0.5px; }";
  html << ".badge {";
  html << "  display: inline-flex;";
  html << "  align-items: center;";
  html << "  gap: 6px;";
  html << "  background: rgba(255, 255, 255, 0.15);";
  html << "  border: 1px solid rgba(255, 255, 255, 0.25);";
  html << "  padding: 4px 12px;";
  html << "  border-radius: 9999px;";
  html << "  font-size: 13px;";
  html << "  font-weight: 500;";
  html << "}";
  html << ".badge-dot {";
  html << "  width: 8px;";
  html << "  height: 8px;";
  html << "  background-color: #34d399;";
  html << "  border-radius: 50%;";
  html << "  display: inline-block;";
  html << "  animation: pulse 2s infinite;";
  html << "}";
  html << "@keyframes pulse {";
  html << "  0% { transform: scale(0.95); opacity: 0.5; }";
  html << "  50% { transform: scale(1.1); opacity: 1; }";
  html << "  100% { transform: scale(0.95); opacity: 0.5; }";
  html << "}";
  html << ".card {";
  html << "  background-color: var(--bg-card);";
  html << "  border-radius: 16px;";
  html << "  padding: 24px;";
  html << "  margin-bottom: 24px;";
  html << "  box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.05), 0 2px 4px -1px rgba(0, 0, 0, 0.02);";
  html << "  border: 1px solid var(--border-color);";
  html << "}";
  html << ".card-title {";
  html << "  font-size: 16px;";
  html << "  font-weight: 600;";
  html << "  margin-bottom: 20px;";
  html << "  color: var(--text-primary);";
  html << "  display: flex;";
  html << "  align-items: center;";
  html << "  gap: 8px;";
  html << "  border-bottom: 1px solid var(--border-color);";
  html << "  padding-bottom: 10px;";
  html << "}";
  html << ".card-error {";
  html << "  border-left: 4px solid var(--color-error);";
  html << "  background-color: #fef2f2;";
  html << "}";
  html << ".error-title { color: var(--color-error); font-weight: 600; border-bottom: none; padding-bottom: 0; margin-bottom: 8px; }";
  html << ".error-msg { color: #b91c1c; font-size: 14px; }";
  html << ".form-group { margin-bottom: 20px; }";
  html << ".form-group:last-child { margin-bottom: 0; }";
  html << ".form-group.checkbox-group { display: flex; align-items: center; gap: 8px; cursor: pointer; user-select: none; margin-top: 16px; }";
  html << ".form-group.checkbox-group input[type=\"checkbox\"] { width: 16px; height: 16px; accent-color: var(--primary-color); margin-top: 0; cursor: pointer; }";
  html << ".form-group.checkbox-group label { margin-bottom: 0; cursor: pointer; }";
  html << "label { display: block; font-size: 14px; font-weight: 500; color: var(--text-secondary); margin-bottom: 8px; }";
  html << ".form-group input:not([type=\"checkbox\"]), .form-group select {";
  html << "  width: 100%;";
  html << "  min-width: 0;"; // Override iOS WebKit's default min-width on datetime-local
  html << "  padding: 10px 14px;";
  html << "  border: 1px solid var(--border-color);";
  html << "  border-radius: 8px;";
  html << "  font-size: 14px;";
  html << "  background-color: #ffffff;";
  html << "  color: var(--text-primary);";
  html << "  transition: border-color 0.2s, box-shadow 0.2s;";
  html << "  outline: none;";
  html << "}";
  html << ".form-group select {";
  html << "  appearance: none;";
  html << "  -webkit-appearance: none;";
  html << "  background-image: url(\"data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24' viewBox='0 0 24 24' fill='none' stroke='%234B5563' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><polyline points='6 9 12 15 18 9'></polyline></svg>\");";
  html << "  background-repeat: no-repeat;";
  html << "  background-position: right 12px center;";
  html << "  background-size: 16px;";
  html << "  padding-right: 36px;";
  html << "}";
  html << "::placeholder { color: var(--text-muted); opacity: 0.8; font-size: 12px; }";
  html << "input[type=\"datetime-local\"] { min-height: 38px; }";
  html << ".form-group input:not([type=\"checkbox\"]):focus, .form-group select:focus {";
  html << "  border-color: var(--primary-color);";
  html << "  box-shadow: 0 0 0 3px rgba(var(--primary-color-rgb), 0.15);";
  html << "}";
  html << ".form-group input:disabled, .form-group select:disabled {";
  html << "  background-color: var(--bg-disabled);";
  html << "  color: var(--text-muted);";
  html << "  cursor: not-allowed;";
  html << "}";
  html << ".wifi-section { margin-bottom: 20px; }";
  html << ".wifi-grid {";
  html << "  display: grid;";
  html << "  grid-template-columns: repeat(auto-fill, minmax(130px, 1fr));";
  html << "  gap: 10px;";
  html << "  margin-top: 8px;";
  html << "  max-height: 160px;";
  html << "  overflow-y: auto;";
  html << "  padding-right: 4px;";
  html << "}";
  html << ".wifi-grid::-webkit-scrollbar { width: 6px; }";
  html << ".wifi-grid::-webkit-scrollbar-thumb { background-color: var(--border-color); border-radius: 3px; }";
  html << ".wifi-pill {";
  html << "  display: flex;";
  html << "  align-items: center;";
  html << "  justify-content: space-between;";
  html << "  padding: 8px 12px;";
  html << "  border: 1px solid var(--border-color);";
  html << "  border-radius: 8px;";
  html << "  background-color: #ffffff;";
  html << "  font-size: 13px;";
  html << "  color: var(--text-primary);";
  html << "  cursor: pointer;";
  html << "  transition: all 0.2s;";
  html << "  text-align: left;";
  html << "  gap: 6px;";
  html << "  outline: none;";
  html << "}";
  html << ".wifi-pill span { white-space: nowrap; overflow: hidden; text-overflow: ellipsis; flex-grow: 1; }";
  html << ".wifi-pill:hover, .wifi-pill.active {";
  html << "  border-color: var(--primary-color);";
  html << "  background-color: rgba(var(--primary-color-rgb), 0.05);";
  html << "  color: var(--primary-color);";
  html << "}";
  html << ".sig-bars { display: inline-flex; align-items: flex-end; gap: 2px; width: 14px; height: 10px; flex-shrink: 0; }";
  html << ".sig-bars .bar { width: 2px; background-color: var(--bg-disabled); border-radius: 1px; }";
  html << ".sig-bars .bar:nth-child(1) { height: 25%; }";
  html << ".sig-bars .bar:nth-child(2) { height: 50%; }";
  html << ".sig-bars .bar:nth-child(3) { height: 75%; }";
  html << ".sig-bars .bar:nth-child(4) { height: 100%; }";
  html << ".hint { font-size: 12px; color: var(--text-secondary); margin-top: 6px; }";
  html << "button[type=\"submit\"] {";
  html << "  width: 100%;";
  html << "  padding: 12px;";
  html << "  background-color: var(--primary-color);";
  html << "  color: #ffffff;";
  html << "  border: none;";
  html << "  border-radius: 8px;";
  html << "  font-size: 15px;";
  html << "  font-weight: 600;";
  html << "  cursor: pointer;";
  html << "  transition: background-color 0.2s, box-shadow 0.2s;";
  html << "  box-shadow: 0 4px 6px -1px rgba(var(--primary-color-rgb), 0.2), 0 2px 4px -1px rgba(var(--primary-color-rgb), 0.1);";
  html << "  margin-top: 16px;";
  html << "  margin-bottom: 24px;";
  html << "}";
  html << "button[type=\"submit\"]:hover { background-color: var(--primary-hover); }";
  html << "button[type=\"submit\"]:focus {";
  html << "  box-shadow: 0 0 0 3px rgba(var(--primary-color-rgb), 0.4);";
  html << "  outline: none;";
  html << "}";
  html << "@keyframes highlight-flash {";
  html << "  0% { background-color: rgba(var(--primary-color-rgb), 0.4); }";
  html << "  100% { background-color: transparent; }";
  html << "}";
  html << ".highlight-flash {";
  html << "  animation: highlight-flash 0.8s ease-out;";
  html << "}";
  html << "@media (max-width: 640px) {";
  html << "  body { padding: 16px 12px; }";
  html << "  .card { padding: 16px; border-radius: 12px; }";
  html << "  .banner { padding: 16px; border-radius: 12px; }";
  html << "  .banner-title { font-size: 18px; }";
  html << "  .wifi-grid { grid-template-columns: repeat(auto-fill, minmax(110px, 1fr)); }";
  html << "  .wifi-pill { padding: 6px 10px; font-size: 12px; }";
  html << "  .form-group input:not([type=\"checkbox\"]), .form-group select {";
  html << "    padding: 10px 8px;";
  html << "    font-size: 13px;";
  html << "    max-width: 100%;";
  html << "  }";
  html << "  button[type=\"submit\"] {";
  html << "    padding: 14px;";
  html << "    font-size: 15px;";
  html << "  }";
  html << "}";
  html << "</style></head><body>";
  html << "<div class=\"container\">";
  html << "<div class=\"banner\">";
  html << "<svg viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\">";
  html << "<path d=\"M5 12.55a11 11 0 0 1 14.08 0\"></path>";
  html << "<path d=\"M1.42 9a16 16 0 0 1 21.16 0\"></path>";
  html << "<path d=\"M8.53 16.11a6 6 0 0 1 6.95 0\"></path>";
  html << "<line x1=\"12\" y1=\"20\" x2=\"12.01\" y2=\"20\"></line>";
  html << "</svg>";
  html << "<div class=\"banner-info\">";
  html << "<div class=\"banner-title\">HomeDeck Setup</div>";
  html << "<div class=\"badge\">";
  html << "<span class=\"badge-dot\"></span>";
  html << "AP Active: <span style=\"font-weight:600;margin-left:2px;\">" << htmlEscape(apSsid) << "</span>";
  html << "</div>";
  html << "</div>";
  html << "</div>";

  html << "<div id=\"error_callout\" class=\"card card-error\"" << (message.empty() ? " style=\"display:none;\"" : "") << ">";
  html << "<div class=\"card-title error-title\">";
  html << "<svg viewBox=\"0 0 24 24\" width=\"18\" height=\"18\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" style=\"margin-right:6px;vertical-align:middle;\">";
  html << "<circle cx=\"12\" cy=\"12\" r=\"10\"></circle>";
  html << "<line x1=\"12\" y1=\"8\" x2=\"12\" y2=\"12\"></line>";
  html << "<line x1=\"12\" y1=\"16\" x2=\"12.01\" y2=\"16\"></line>";
  html << "</svg>";
  html << "配置错误";
  html << "</div>";
  html << "<div class=\"error-msg\" id=\"error_msg\">" << (message.empty() ? "" : htmlEscape(message)) << "</div>";
  html << "</div>";

  html << "<form id=\"setup_form\" method=\"post\" action=\"/save\">";
  html << "<div class=\"card\">";
  html << "<div class=\"card-title\">";
  html << "<svg viewBox=\"0 0 24 24\" width=\"18\" height=\"18\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" style=\"margin-right:6px;vertical-align:middle;\">";
  html << "<path d=\"M5 12.55a11 11 0 0 1 14.08 0\"></path>";
  html << "<line x1=\"12\" y1=\"20\" x2=\"12.01\" y2=\"20\"></line>";
  html << "</svg>";
  html << "网络与系统配置";
  html << "</div>";

  html << "<div class=\"wifi-section\">";
  html << "<label>可用 Wi-Fi 列表</label>";
  html << "<div class=\"wifi-grid\">";
  for (const auto& network : networks) {
    int rssi = network.rssi;
    int activeBars = 1;
    std::string barColor = "var(--color-warning)";
    if (rssi >= -55) {
      activeBars = 4;
      barColor = "var(--color-success)";
    } else if (rssi >= -70) {
      activeBars = 3;
      barColor = "var(--color-success)";
    } else if (rssi >= -85) {
      activeBars = 2;
      barColor = "var(--color-warning)";
    } else {
      activeBars = 1;
      barColor = "var(--color-warning)";
    }

    html << "<button type=\"button\" class=\"wifi-pill\" data-ssid=\"" << htmlEscape(network.ssid) << "\">";
    html << "<span>" << htmlEscape(network.ssid) << "</span>";
    html << "<div class=\"sig-bars\">";
    for (int b = 1; b <= 4; ++b) {
      html << "<div class=\"bar\"";
      if (b <= activeBars) {
        html << " style=\"background-color:" << barColor << ";\"";
      }
      html << "></div>";
    }
    html << "</div>";
    html << "</button>";
  }
  html << "</div>";
  html << "</div>";

  html << "<div class=\"form-group\">";
  html << "<label for=\"wifi_ssid\">Wi-Fi SSID</label>";
  html << "<input type=\"text\" id=\"wifi_ssid\" name=\"wifi_ssid\" value=\"" << htmlEscape(values.wifiSsid) << "\" placeholder=\"选择或输入 SSID\">";
  html << "</div>";

  html << "<div class=\"form-group\">";
  html << "<label for=\"wifi_password\">Wi-Fi 密码</label>";
  html << "<input type=\"password\" id=\"wifi_password\" name=\"wifi_password\" value=\"" << htmlEscape(values.wifiPassword) << "\" placeholder=\"请输入密码\">";
  html << "<div class=\"form-group checkbox-group\" style=\"margin-top:8px;\">";
  html << "<input type=\"checkbox\" id=\"show_password\">";
  html << "<label for=\"show_password\" style=\"margin-bottom:0;cursor:pointer;\">显示密码</label>";
  html << "</div>";
  html << "</div>";

  html << "<div class=\"form-group\">";
  html << "<label for=\"timezone\">时区</label>";
  html << "<select id=\"timezone\" name=\"timezone\">";
  std::size_t timezoneCount = 0;
  const auto* timezones = timezoneCatalog(&timezoneCount);
  for (std::size_t i = 0; i < timezoneCount; ++i) {
    html << "<option value=\"" << timezones[i].iana << "\"";
    if (values.timezoneIana == timezones[i].iana) {
      html << " selected";
    }
    html << ">" << timezones[i].label << "</option>";
  }
  html << "</select>";
  html << "</div>";

  html << "<div class=\"form-group checkbox-group\">";
  html << "<input id=\"auto_rtc\" name=\"auto_rtc\" type=\"checkbox\" value=\"1\"";
  if (values.autoRtcCorrection) {
    html << " checked";
  }
  if (values.wifiSsid.empty()) {
    html << " disabled";
  }
  html << ">";
  html << "<label for=\"auto_rtc\" style=\"margin-bottom:0;cursor:pointer;\">自动纠正 RTC</label>";
  html << "</div>";

  html << "<div class=\"form-group\" style=\"margin-top:16px;\">";
  html << "<label for=\"ntp_server\">NTP 服务器</label>";
  html << "<input type=\"text\" id=\"ntp_server\" name=\"ntp_server\" value=\"" << htmlEscape(values.ntpServer) << "\" placeholder=\"pool.ntp.org\">";
  html << "</div>";

  html << "<div class=\"form-group\">";
  html << "<label for=\"manual_datetime\">手动日期时间</label>";
  html << "<input type=\"datetime-local\" id=\"manual_datetime\" name=\"manual_datetime\">";
  html << "</div>";
  html << "</div>";

  html << "<div class=\"card\">";
  html << "<div class=\"card-title\">";
  html << "<svg viewBox=\"0 0 24 24\" width=\"18\" height=\"18\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" style=\"margin-right:6px;vertical-align:middle;\">";
  html << "<path d=\"M21 10c0 7-9 13-9 13s-9-6-9-13a9 9 0 0 1 18 0z\"></path>";
  html << "<circle cx=\"12\" cy=\"10\" r=\"3\"></circle>";
  html << "</svg>";
  html << "地理位置配置";
  html << "</div>";

  html << "<div class=\"form-group\">";
  html << "<label for=\"latitude\">纬度</label>";
  html << "<input type=\"text\" id=\"latitude\" name=\"latitude\" value=\"" << htmlEscape(values.latitude) << "\">";
  html << "</div>";

  html << "<div class=\"form-group\">";
  html << "<label for=\"longitude\">经度</label>";
  html << "<input type=\"text\" id=\"longitude\" name=\"longitude\" value=\"" << htmlEscape(values.longitude) << "\">";
  html << "</div>";

  html << "<div class=\"form-group\">";
  html << "<label for=\"osm_link\">粘贴 OpenStreetMap 链接</label>";
  html << "<input type=\"url\" id=\"osm_link\" placeholder=\"https://www.openstreetmap.org/#map=19/25.7817/113.0199\">";
  html << "</div>";
  html << "<div class=\"callout callout-info hint\"><svg class=\"callout-icon\" width=\"16\" height=\"16\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z\" /></svg>";
  html << "<p class=\"hint\">打开 openstreetmap.org，定位到当前位置后复制浏览器地址栏的链接粘贴到此处，可自动提取经纬度。</p></div>";
  html << "</div>";

  html << "<button type=\"submit\" id=\"submit_btn\">保存配置</button>";
  html << "</form>";
  html << "</div>";

  html << "<script>";
  html << "const ssidInput = document.getElementById('wifi_ssid');";
  html << "const autoRtc = document.getElementById('auto_rtc');";
  html << "const latInput = document.getElementById('latitude');";
  html << "const lonInput = document.getElementById('longitude');";
  html << "const osmInput = document.getElementById('osm_link');";
  html << "const errCallout = document.getElementById('error_callout');";
  html << "const errMsg = document.getElementById('error_msg');";
  html << "const submitBtn = document.getElementById('submit_btn');";
  html << "const setupForm = document.getElementById('setup_form');";
  html << "const wifiPills = document.querySelectorAll('.wifi-pill');";
  html << "const manualDatetime = document.getElementById('manual_datetime');";
  html << "const wifiPassword = document.getElementById('wifi_password');";
  html << "const showPassword = document.getElementById('show_password');";
  html << "function sync() {";
  html << "  autoRtc.disabled = ssidInput.value.trim() === '';";
  html << "  if (autoRtc.disabled) autoRtc.checked = false;";
  html << "  manualDatetime.disabled = autoRtc.checked && !autoRtc.disabled;";
  html << "  if (manualDatetime.disabled) manualDatetime.value = '';";
  html << "}";
  html << "function pickSsid(btn) {";
  html << "  wifiPills.forEach(b => b.classList.remove('active'));";
  html << "  btn.classList.add('active');";
  html << "  ssidInput.value = btn.dataset.ssid;";
  html << "  sync();";
  html << "}";
  html << "wifiPills.forEach(b => {";
  html << "  b.addEventListener('click', () => pickSsid(b));";
  html << "  if (b.dataset.ssid === ssidInput.value) {";
  html << "    b.classList.add('active');";
  html << "  }";
  html << "});";
  html << "ssidInput.addEventListener('input', sync);";
  html << "autoRtc.addEventListener('change', sync);";
  html << "sync();";
  html << "showPassword.addEventListener('change', function() {";
  html << "  wifiPassword.type = this.checked ? 'text' : 'password';";
  html << "});";
  html << "osmInput.addEventListener('input', function() {";
  html << "  const val = this.value.trim();";
  html << "  let lat = '';";
  html << "  let lon = '';";
  html << "  const m = val.match(/#map=[0-9.]+\\/([0-9.-]+)\\/([0-9.-]+)/);";
  html << "  if (m) {";
  html << "    lat = m[1];";
  html << "    lon = m[2];";
  html << "  } else {";
  html << "    const latMatch = val.match(/[?&](?:mlat|lat)=([0-9.-]+)/);";
  html << "    const lonMatch = val.match(/[?&](?:mlon|lon)=([0-9.-]+)/);";
  html << "    if (latMatch && lonMatch) {";
  html << "      lat = latMatch[1];";
  html << "      lon = lonMatch[1];";
  html << "    }";
  html << "  }";
  html << "  if (lat && lon) {";
  html << "    latInput.value = lat;";
  html << "    lonInput.value = lon;";
  html << "    latInput.classList.remove('highlight-flash');";
  html << "    lonInput.classList.remove('highlight-flash');";
  html << "    void latInput.offsetWidth;";
  html << "    void lonInput.offsetWidth;";
  html << "    latInput.classList.add('highlight-flash');";
  html << "    lonInput.classList.add('highlight-flash');";
  html << "    errCallout.style.display = 'none';";
  html << "    errMsg.textContent = '';";
  html << "  }";
  html << "});";
  html << "setupForm.addEventListener('submit', function(e) {";
  html << "  const latVal = latInput.value.trim();";
  html << "  const lonVal = lonInput.value.trim();";
  html << "  let error = '';";
  html << "  if (!latVal) {";
  html << "    error = '纬度不能为空';";
  html << "  } else if (isNaN(Number(latVal))) {";
  html << "    error = '纬度必须是合法的数字';";
  html << "  } else if (Number(latVal) < -90 || Number(latVal) > 90) {";
  html << "    error = '纬度范围必须在 -90 到 90 之间';";
  html << "  } else if (!lonVal) {";
  html << "    error = '经度不能为空';";
  html << "  } else if (isNaN(Number(lonVal))) {";
  html << "    error = '经度必须是合法的数字';";
  html << "  } else if (Number(lonVal) < -180 || Number(lonVal) > 180) {";
  html << "    error = '经度范围必须在 -180 到 180 之间';";
  html << "  }";
  html << "  if (error) {";
  html << "    e.preventDefault();";
  html << "    errMsg.textContent = error;";
  html << "    errCallout.style.display = 'block';";
  html << "    window.scrollTo({ top: 0, behavior: 'smooth' });";
  html << "    return;";
  html << "  }";
  html << "  submitBtn.disabled = true;";
  html << "  submitBtn.textContent = '正在保存...';";
  html << "});";
  html << "</script>";
  html << "</body></html>";
  return html.str();
}

}  // namespace homedeck
