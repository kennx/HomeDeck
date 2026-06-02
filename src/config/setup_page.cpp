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
  html << "<style>body{font-family:sans-serif;margin:24px;max-width:680px}label{display:block;margin-top:14px}input,select,button{font-size:16px;padding:8px;width:100%;box-sizing:border-box}.wifi button{margin:4px 0}.msg{color:#b00020}#location_status{display:block;margin-top:4px;font-size:14px}#location_status.ok{color:#2e7d32}#location_status.err{color:#b00020}</style>";
  html << "</head><body><h1>HomeDeck Setup</h1>";
  html << "<p>AP: " << htmlEscape(apSsid) << " / 192.168.4.1</p>";
  if (!message.empty()) {
    html << "<p class=\"msg\">" << htmlEscape(message) << "</p>";
  }
  html << "<div class=\"wifi\"><strong>Wi-Fi 列表</strong>";
  for (const auto& network : networks) {
    html << "<button type=\"button\" data-ssid=\"" << htmlEscape(network.ssid) << "\">";
    html << htmlEscape(network.ssid) << " (" << network.rssi << " dBm)</button>";
  }
  html << "</div><form method=\"post\" action=\"/save\">";
  html << "<label>Wi-Fi SSID<input id=\"wifi_ssid\" name=\"wifi_ssid\" value=\"" << htmlEscape(values.wifiSsid) << "\"></label>";
  html << "<label>Wi-Fi 密码<input name=\"wifi_password\" type=\"password\" value=\"" << htmlEscape(values.wifiPassword) << "\"></label>";
  html << "<label>时区<select name=\"timezone\">";
  std::size_t timezoneCount = 0;
  const auto* timezones = timezoneCatalog(&timezoneCount);
  for (std::size_t i = 0; i < timezoneCount; ++i) {
    html << "<option value=\"" << timezones[i].iana << "\"";
    if (values.timezoneIana == timezones[i].iana) {
      html << " selected";
    }
    html << ">" << timezones[i].label << "</option>";
  }
  html << "</select></label>";
  html << "<label><input id=\"auto_rtc\" name=\"auto_rtc\" type=\"checkbox\" value=\"1\"";
  if (values.autoRtcCorrection) {
    html << " checked";
  }
  if (values.wifiSsid.empty()) {
    html << " disabled";
  }
  html << "> 自动纠正 RTC</label>";
  html << "<label>NTP 服务器<input name=\"ntp_server\" value=\"" << htmlEscape(values.ntpServer) << "\"></label>";
  html << "<label>手动日期时间<input name=\"manual_datetime\" type=\"datetime-local\"></label>";
  html << "<label>纬度 <input id=\"latitude\" name=\"latitude\" value=\"" << htmlEscape(values.latitude) << "\"></label>";
  html << "<label>经度 <input id=\"longitude\" name=\"longitude\" value=\"" << htmlEscape(values.longitude) << "\"></label>";
  html << "<button type=\"button\" id=\"get_location\">获取当前位置</button><span id=\"location_status\"></span>";
  html << "<button type=\"submit\">保存</button></form>";
  html << "<script>const ssid=document.getElementById('wifi_ssid');const auto=document.getElementById('auto_rtc');function sync(){auto.disabled=ssid.value.trim()==='';if(auto.disabled)auto.checked=false;}function pickSsid(v){ssid.value=v;sync();}document.querySelectorAll('.wifi button[data-ssid]').forEach(b=>b.addEventListener('click',()=>pickSsid(b.dataset.ssid)));ssid.addEventListener('input',sync);sync();const latInput=document.getElementById('latitude');const lonInput=document.getElementById('longitude');const locStatus=document.getElementById('location_status');document.getElementById('get_location').addEventListener('click',function(){if(!navigator.geolocation){locStatus.textContent='浏览器不支持地理定位';locStatus.className='err';return;}locStatus.textContent='正在获取位置...';locStatus.className='';navigator.geolocation.getCurrentPosition(function(pos){latInput.value=pos.coords.latitude.toFixed(6);lonInput.value=pos.coords.longitude.toFixed(6);locStatus.textContent='位置已获取';locStatus.className='ok';},function(err){var msgs={1:'权限被拒绝',2:'位置不可用',3:'获取超时'};locStatus.textContent=msgs[err.code]||'获取失败';locStatus.className='err';},{timeout:10000,enableHighAccuracy:false});});</script>";
  html << "</body></html>";
  return html.str();
}

}  // namespace homedeck
