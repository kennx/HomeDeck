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
  html << "  --primary-color: #4F46E5;";
  html << "  --primary-hover: #4338CA;";
  html << "  --primary-light: #EEF2FF;";
  html << "  --bg-page: #F4F5F7;";
  html << "  --bg-card: #FFFFFF;";
  html << "  --bg-input: #F9FAFB;";
  html << "  --bg-input-focus: #FFFFFF;";
  html << "  --color-success: #10B981;";
  html << "  --color-success-bg: #ECFDF5;";
  html << "  --color-error: #EF4444;";
  html << "  --color-error-bg: #FEF2F2;";
  html << "  --color-error-border: #FCA5A5;";
  html << "  --color-info: #3B82F6;";
  html << "  --color-info-bg: #EFF6FF;";
  html << "  --color-info-border: #BFDBFE;";
  html << "  --text-primary: #1F2937;";
  html << "  --text-secondary: #4B5563;";
  html << "  --text-tertiary: #9CA3AF;";
  html << "  --text-on-primary: #FFFFFF;";
  html << "  --radius-card: 16px;";
  html << "  --radius-input: 8px;";
  html << "  --radius-badge: 6px;";
  html << "  --border-color: #E5E7EB;";
  html << "  --border-color-focus: #4F46E5;";
  html << "  --border-width: 1px;";
  html << "  --space-xs: 4px;";
  html << "  --space-sm: 8px;";
  html << "  --space-md: 16px;";
  html << "  --space-lg: 24px;";
  html << "  --space-xl: 32px;";
  html << "  --shadow-card: 0 4px 6px -1px rgba(0, 0, 0, 0.03), 0 2px 4px -1px rgba(0, 0, 0, 0.02), 0 16px 24px -4px rgba(0, 0, 0, 0.04);";
  html << "  --shadow-focus: 0 0 0 3px rgba(79, 70, 229, 0.15);";
  html << "  --transition-fast: 0.15s ease;";
  html << "  --primary-color-rgb: 79, 70, 229;";
  html << "}";
  html << "* { box-sizing: border-box; margin: 0; padding: 0; }";
  html << "body { font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", Roboto, sans-serif; background-color: var(--bg-page); color: var(--text-primary); padding: var(--space-xl) var(--space-md); display: flex; justify-content: center; min-height: 100vh; }";
  html << ".container { width: 100%; max-width: 600px; display: flex; flex-direction: column; gap: var(--space-md); }";
  html << ".card { background-color: var(--bg-card); border-radius: var(--radius-card); padding: var(--space-lg); box-shadow: var(--shadow-card); border: var(--border-width) solid var(--border-color); }";
  html << ".header-card { display: flex; flex-direction: column; gap: var(--space-sm); }";
  html << ".logo-area { display: flex; align-items: center; gap: var(--space-sm); }";
  html << ".logo-icon { width: 32px; height: 32px; color: var(--primary-color); }";
  html << "h1 { font-size: 24px; font-weight: 700; color: var(--text-primary); letter-spacing: -0.5px; }";
  html << ".ap-status { display: flex; align-items: center; gap: var(--space-sm); flex-wrap: wrap; margin-top: var(--space-xs); }";
  html << ".badge { font-size: 12px; font-weight: 600; padding: 3px 8px; border-radius: var(--radius-badge); text-transform: uppercase; }";
  html << ".badge-success { background-color: var(--color-success-bg); color: var(--color-success); border: 1px solid rgba(16, 185, 129, 0.2); }";
  html << ".ap-info { font-size: 14px; color: var(--text-secondary); }";
  html << ".callout { padding: var(--space-md); border-radius: var(--radius-input); display: flex; gap: var(--space-sm); align-items: flex-start; }";
  html << ".callout-error { background-color: var(--color-error-bg); border: var(--border-width) solid var(--color-error-border); color: var(--color-error); }";
  html << ".callout-info { background-color: var(--color-info-bg); border: var(--border-width) solid var(--color-info-border); color: var(--color-info); }";
  html << ".callout-icon { flex-shrink: 0; margin-top: 2px; }";
  html << ".callout p { font-size: 13px; line-height: 1.5; }";
  html << ".callout-info p { color: var(--text-secondary); }";
  html << ".callout-error p { font-weight: 500; }";
  html << "h2 { font-size: 16px; font-weight: 600; margin-bottom: var(--space-md); display: flex; align-items: center; gap: var(--space-sm); color: var(--text-primary); }";
  html << ".section-icon { width: 18px; height: 18px; color: var(--text-secondary); }";
  html << ".wifi-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(130px, 1fr)); gap: var(--space-sm); }";
  html << ".wifi-pill { border: var(--border-width) solid var(--border-color); background-color: var(--bg-card); border-radius: var(--radius-badge); padding: var(--space-sm) var(--space-md); font-size: 13px; font-weight: 500; cursor: pointer; display: flex; flex-direction: column; align-items: flex-start; gap: 2px; text-align: left; transition: all var(--transition-fast); color: var(--text-secondary); }";
  html << ".wifi-pill:hover, .wifi-pill.active { border-color: var(--primary-color); background-color: var(--primary-light); color: var(--primary-color); }";
  html << ".wifi-pill.active { box-shadow: var(--shadow-focus); }";
  html << ".wifi-meta { font-size: 11px; color: var(--text-tertiary); display: flex; align-items: center; gap: 4px; }";
  html << ".wifi-pill:hover .wifi-meta, .wifi-pill.active .wifi-meta { color: var(--primary-color); }";
  html << ".sig-bars { display: inline-flex; align-items: flex-end; gap: 2px; width: 14px; height: 10px; flex-shrink: 0; }";
  html << ".sig-bars .sig-bar { width: 2px; background-color: var(--bg-disabled); border-radius: 1px; transition: background-color var(--transition-fast); }";
  html << ".sig-bars .sig-bar:nth-child(1) { height: 25%; }";
  html << ".sig-bars .sig-bar:nth-child(2) { height: 50%; }";
  html << ".sig-bars .sig-bar:nth-child(3) { height: 75%; }";
  html << ".sig-bars .sig-bar:nth-child(4) { height: 100%; }";
  html << ".wifi-pill:hover .sig-bar, .wifi-pill.active .sig-bar { background-color: var(--primary-color); }";
  html << ".wifi-pill[data-sig=\"good\"] .sig-bar { background-color: var(--color-success); }";
  html << ".wifi-pill[data-sig=\"mid\"] .sig-bar:nth-child(1), .wifi-pill[data-sig=\"mid\"] .sig-bar:nth-child(2), .wifi-pill[data-sig=\"mid\"] .sig-bar:nth-child(3) { background-color: #F59E0B; }";
  html << ".wifi-pill[data-sig=\"weak\"] .sig-bar:nth-child(1) { background-color: var(--color-error); }";
  html << "form { display: flex; flex-direction: column; gap: var(--space-md); }";
  html << ".form-group { margin-bottom: var(--space-md); }";
  html << ".form-group.checkbox-group { display: flex; align-items: center; gap: var(--space-sm); cursor: pointer; user-select: none; margin-top: var(--space-md); }";
  html << ".form-group.checkbox-group input[type=\"checkbox\"] { width: 16px; height: 16px; accent-color: var(--primary-color); margin-top: 0; cursor: pointer; }";
  html << ".form-group.checkbox-group label { margin-bottom: 0; cursor: pointer; }";
  html << "label { display: block; font-size: 14px; font-weight: 500; color: var(--text-secondary); margin-bottom: var(--space-sm); }";
  html << ".form-group input:not([type=\"checkbox\"]), .form-group select { width: 100%; min-width: 0; padding: 10px 14px; border: var(--border-width) solid var(--border-color); border-radius: var(--radius-input); font-size: 14px; background-color: #ffffff; color: var(--text-primary); transition: border-color var(--transition-fast), box-shadow var(--transition-fast); outline: none; }";
  html << ".form-group input:not([type=\"checkbox\"]):focus, .form-group select:focus { border-color: var(--border-color-focus); box-shadow: var(--shadow-focus); }";
  html << ".form-group input:disabled, .form-group select:disabled { background-color: var(--bg-disabled); color: var(--text-tertiary); cursor: not-allowed; }";
  html << ".form-group select { appearance: none; -webkit-appearance: none; background-image: url(\"data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24' viewBox='0 0 24 24' fill='none' stroke='%234B5563' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><polyline points='6 9 12 15 18 9'></polyline></svg>\"); background-repeat: no-repeat; background-position: right 12px center; background-size: 16px; padding-right: 36px; }";
  html << "::placeholder { color: var(--text-muted); opacity: 0.8; font-size: 12px; }";
  html << "input[type=\"datetime-local\"] { min-height: 38px; }";
  html << ".btn-submit { background-color: var(--primary-color); color: var(--text-on-primary); font-size: 15px; font-weight: 600; padding: var(--space-md); border: none; border-radius: var(--radius-input); cursor: pointer; transition: all var(--transition-fast); box-shadow: 0 4px 6px -1px rgba(var(--primary-color-rgb), 0.2); width: 100%; margin-top: 16px; margin-bottom: 24px; }";
  html << ".btn-submit:hover:not(:disabled) { background-color: var(--primary-hover); box-shadow: 0 6px 12px -2px rgba(var(--primary-color-rgb), 0.3); }";
  html << ".btn-submit:active:not(:disabled) { transform: scale(0.98); }";
  html << ".btn-submit:disabled { background-color: var(--text-tertiary); cursor: not-allowed; box-shadow: none; }";
  html << "@keyframes flash-green { 0% { background-color: var(--color-success-bg); border-color: var(--color-success); } 100% { background-color: var(--bg-input-focus); border-color: var(--border-color-focus); } }";
  html << ".highlight-flash { animation: flash-green 0.8s ease-out; }";
  html << "@media (max-width: 640px) {";
  html << "  body { padding: 16px 12px; }";
  html << "  .card { padding: 16px; border-radius: 12px; }";
  html << "  .banner { padding: 16px; border-radius: 12px; }";
  html << "  .banner-title { font-size: 18px; }";
  html << "  .wifi-grid { grid-template-columns: repeat(auto-fill, minmax(110px, 1fr)); }";
  html << "  .wifi-pill { padding: 6px 10px; font-size: 12px; }";
  html << "  .form-group input:not([type=\"checkbox\"]), .form-group select { padding: 10px 8px; font-size: 13px; max-width: 100%; }";
  html << "  .btn-submit { padding: 14px; font-size: 15px; }";
  html << "}";
  html << "</style></head><body>";
  html << "<div class=\"container\">";
  html << "<header class=\"card header-card\">";
  html << "<div class=\"logo-area\">";
  html << "<svg class=\"logo-icon\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z\" /><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M15 12a3 3 0 11-6 0 3 3 0 016 0z\" /></svg>";
  html << "<h1>HomeDeck Setup</h1>";
  html << "</div>";
  html << "<div class=\"ap-status\">";
  html << "<span class=\"badge badge-success\">AP 运行中</span>";
  html << "<span class=\"ap-info\">热点: <strong>" << htmlEscape(apSsid) << "</strong> / 192.168.4.1</span>";
  html << "</div>";
  html << "</header>";

  if (!message.empty()) {
    html << "<div id=\"error_container\" class=\"callout callout-error\">";
    html << "<svg class=\"callout-icon\" width=\"16\" height=\"16\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z\" /></svg>";
    html << "<p class=\"msg\">" << htmlEscape(message) << "</p>";
    html << "</div>";
  } else {
    html << "<div id=\"error_container\" class=\"callout callout-error\" style=\"display:none;\">";
    html << "<svg class=\"callout-icon\" width=\"16\" height=\"16\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z\" /></svg>";
    html << "<p id=\"error_msg\" class=\"msg\"></p>";
    html << "</div>";
  }

  html << "<section class=\"card\">";
  html << "<h2>";
  html << "<svg class=\"section-icon\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M8.111 16.404a5.5 5.5 0 017.778 0M12 20h.01m-7.08-7.071a9 9 0 0112.14 0M1.93 9.143a13 13 0 0118.14 0\" /></svg>";
  html << "Wi-Fi 列表</h2>";
  html << "<div class=\"wifi-grid\">";
  for (const auto& network : networks) {
    std::string signalQuality = "good";
    if (network.rssi < -80) signalQuality = "weak";
    else if (network.rssi < -65) signalQuality = "mid";

    html << "<button type=\"button\" class=\"wifi-pill\" data-ssid=\"" << htmlEscape(network.ssid) << "\" data-sig=\"" << signalQuality << "\">";
    html << "<span>" << htmlEscape(network.ssid) << "</span>";
    html << "<div class=\"wifi-meta\">";
    html << "<div class=\"sig-bars\"><span class=\"sig-bar\"></span><span class=\"sig-bar\"></span><span class=\"sig-bar\"></span><span class=\"sig-bar\"></span></div>";
    html << network.rssi << " dBm</div></button>";
  }
  html << "</div></section>";

  html << "<form id=\"setup_form\" method=\"post\" action=\"/save\">";
  
  html << "<section class=\"card\">";
  html << "<h2><svg class=\"section-icon\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4\" /></svg>";
  html << "网络与系统配置</h2>";
  
  html << "<div class=\"form-group\"><label for=\"wifi_ssid\">Wi-Fi SSID</label>";
  html << "<input type=\"text\" id=\"wifi_ssid\" name=\"wifi_ssid\" value=\"" << htmlEscape(values.wifiSsid) << "\" placeholder=\"请输入 SSID 或从上方选择\"></div>";
  
  html << "<div class=\"form-group\"><label for=\"wifi_password\">Wi-Fi 密码</label>";
  html << "<input id=\"wifi_password\" name=\"wifi_password\" type=\"password\" value=\"" << htmlEscape(values.wifiPassword) << "\" placeholder=\"请输入无线密码\">";
  html << "<div class=\"form-group checkbox-group\" style=\"margin-top:8px;\">";
  html << "<input type=\"checkbox\" id=\"show_password\">";
  html << "<label for=\"show_password\" style=\"margin-bottom:0;cursor:pointer;\">显示密码</label></div></div>";
  
  html << "<div class=\"form-group\"><label for=\"timezone\">时区</label><select id=\"timezone\" name=\"timezone\">";
  std::size_t timezoneCount = 0;
  const auto* timezones = timezoneCatalog(&timezoneCount);
  for (std::size_t i = 0; i < timezoneCount; ++i) {
    html << "<option value=\"" << timezones[i].iana << "\"";
    if (values.timezoneIana == timezones[i].iana) {
      html << " selected";
    }
    html << ">" << timezones[i].label << "</option>";
  }
  html << "</select></div>";
  
  html << "<div class=\"form-group checkbox-group\">";
  html << "<input id=\"auto_rtc\" name=\"auto_rtc\" type=\"checkbox\" value=\"1\"";
  if (values.autoRtcCorrection) {
    html << " checked";
  }
  if (values.wifiSsid.empty()) {
    html << " disabled";
  }
  html << "> <label for=\"auto_rtc\">自动纠正 RTC</label></div>";
  
  html << "<div class=\"form-group\"><label for=\"ntp_server\">NTP 服务器</label>";
  html << "<input type=\"text\" id=\"ntp_server\" name=\"ntp_server\" value=\"" << htmlEscape(values.ntpServer) << "\" placeholder=\"ntp.aliyun.com\"></div>";
  
  html << "<div class=\"form-group\"><label for=\"manual_datetime\">手动日期时间</label>";
  html << "<input id=\"manual_datetime\" name=\"manual_datetime\" type=\"datetime-local\"></div>";
  html << "</section>";

  html << "<section class=\"card\">";
  html << "<h2><svg class=\"section-icon\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M17.657 16.657L13.414 20.9a1.998 1.998 0 01-2.827 0l-4.244-4.243a8 8 0 1111.314 0z\" /><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M15 11a3 3 0 11-6 0 3 3 0 016 0z\" /></svg>";
  html << "地理位置坐标</h2>";
  
  html << "<div class=\"form-group\"><label for=\"latitude\">纬度</label>";
  html << "<input type=\"text\" id=\"latitude\" name=\"latitude\" value=\"" << htmlEscape(values.latitude) << "\" placeholder=\"例如: 31.2304\"></div>";
  
  html << "<div class=\"form-group\"><label for=\"longitude\">经度</label>";
  html << "<input type=\"text\" id=\"longitude\" name=\"longitude\" value=\"" << htmlEscape(values.longitude) << "\" placeholder=\"例如: 121.4737\"></div>";
  
  html << "<div class=\"form-group\"><label for=\"osm_link\">OpenStreetMap 地图链接</label>";
  html << "<input type=\"url\" id=\"osm_link\" placeholder=\"在此粘贴 OpenStreetMap 链接以提取坐标\"></div>";
  
  html << "<div class=\"callout callout-info hint\"><svg class=\"callout-icon\" width=\"16\" height=\"16\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\"><path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z\" /></svg>";
  html << "<p class=\"hint\">打开 openstreetmap.org，定位当前位置并复制浏览器地址栏的链接粘贴至上方，可自动提取经纬度。</p></div>";
  html << "</section>";

  html << "<button type=\"submit\" id=\"submit_btn\" class=\"btn-submit\">保存配置</button>";
  html << "</form></div>";

  html << "<script>";
  html << "const ssidInput = document.getElementById('wifi_ssid');";
  html << "const autoRtc = document.getElementById('auto_rtc');";
  html << "const latInput = document.getElementById('latitude');";
  html << "const lonInput = document.getElementById('longitude');";
  html << "const osmInput = document.getElementById('osm_link');";
  html << "const errCallout = document.getElementById('error_container');";
  html << "const errMsg = document.getElementById('error_msg');";
  html << "const submitBtn = document.getElementById('submit_btn');";
  html << "const setupForm = document.getElementById('setup_form');";
  html << "const wifiPills = document.querySelectorAll('.wifi-pill');";
  html << "const manualDatetime = document.getElementById('manual_datetime');";
  html << "const wifiPassword = document.getElementById('wifi_password');";
  html << "const showPassword = document.getElementById('show_password');";

  html << "function syncRtcState() {";
  html << "  autoRtc.disabled = ssidInput.value.trim() === '';";
  html << "  if (autoRtc.disabled) autoRtc.checked = false;";
  html << "  manualDatetime.disabled = autoRtc.checked && !autoRtc.disabled;";
  html << "  if (manualDatetime.disabled) manualDatetime.value = '';";
  html << "}";

  html << "function selectWifi(pill, ssidValue) {";
  html << "  wifiPills.forEach(p => p.classList.remove('active'));";
  html << "  pill.classList.add('active');";
  html << "  ssidInput.value = ssidValue;";
  html << "  syncRtcState();";
  html << "}";

  html << "wifiPills.forEach(pill => {";
  html << "  pill.addEventListener('click', () => selectWifi(pill, pill.dataset.ssid));";
  html << "  if (pill.dataset.ssid === ssidInput.value) {";
  html << "    pill.classList.add('active');";
  html << "  }";
  html << "});";

  html << "ssidInput.addEventListener('input', () => {";
  html << "  syncRtcState();";
  html << "  wifiPills.forEach(p => {";
  html << "    if (p.dataset.ssid === ssidInput.value) p.classList.add('active');";
  html << "    else p.classList.remove('active');";
  html << "  });";
  html << "});";

  html << "autoRtc.addEventListener('change', syncRtcState);";
  html << "showPassword.addEventListener('change', function() {";
  html << "  wifiPassword.type = this.checked ? 'text' : 'password';";
  html << "});";

  html << "osmInput.addEventListener('input', function() {";
  html << "  const val = this.value.trim();";
  html << "  let latVal = '', lonVal = '';";
  html << "  const hashMatch = val.match(/#map=[0-9.]+\\/([0-9.-]+)\\/([0-9.-]+)/);";
  html << "  if (hashMatch) {";
  html << "    latVal = hashMatch[1];";
  html << "    lonVal = hashMatch[2];";
  html << "  } else {";
  html << "    const latMatch = val.match(/[?&](?:mlat|lat)=([0-9.-]+)/);";
  html << "    const lonMatch = val.match(/[?&](?:mlon|lon)=([0-9.-]+)/);";
  html << "    if (latMatch && lonMatch) {";
  html << "      latVal = latMatch[1];";
  html << "      lonVal = lonMatch[1];";
  html << "    }";
  html << "  }";
  html << "  if (latVal && lonVal) {";
  html << "    latInput.value = latVal;";
  html << "    lonInput.value = lonVal;";
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
  html << "    errCallout.style.display = 'flex';";
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
