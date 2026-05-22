#include "ap_config_portal.h"

#include <ESP.h>
#include <WiFi.h>

#include "homedeck/setup_page.h"
#include "homedeck/config_validator.h"

namespace {

homedeck::SetupConfig readSetupConfig(WebServer& server) {
  return homedeck::SetupConfig{
      server.arg("wifi_ssid").c_str(),
      server.arg("wifi_password").c_str(),
      server.arg("timezone").c_str(),
      server.arg("ntp_server").c_str(),
      homedeck::normalizeCalendarUrl(server.arg("personal_calendar_url").c_str()),
      homedeck::normalizeCalendarUrl(server.arg("holiday_calendar_url").c_str()),
  };
}

}  // namespace

void ApConfigPortal::begin(
    const char* apSsid,
    const homedeck::SetupConfig& defaults,
    SaveCallback onSave) {
  apSsid_ = apSsid;
  defaults_ = defaults;
  onSave_ = std::move(onSave);
  restartScheduled_ = false;
  restartAtMs_ = 0;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid);
  delay(100);

  server_.on("/", HTTP_GET, [this]() { handleIndex(); });
  server_.on("/save", HTTP_POST, [this]() { handleSave(); });
  server_.begin();
}

void ApConfigPortal::handleClient() {
  server_.handleClient();

  if (restartScheduled_ && static_cast<long>(millis() - restartAtMs_) >= 0) {
    ESP.restart();
  }
}

void ApConfigPortal::handleIndex() {
  sendPage(200, defaults_, "");
}

void ApConfigPortal::handleSave() {
  const homedeck::SetupConfig submitted = readSetupConfig(server_);
  const homedeck::ValidationError validation = homedeck::validateSetupConfig(submitted);

  if (validation != homedeck::ValidationError::None) {
    sendPage(400, submitted, homedeck::validationMessage(validation));
    return;
  }

  if (onSave_) {
    int saveResult = onSave_(submitted);
    if (saveResult == 1) {
      sendPage(500, submitted, "保存配置失败：无法连接到 Wi-Fi。");
      return;
    } else if (saveResult == 2) {
      sendPage(500, submitted, "保存配置失败：无法写入存储 (NVS)。");
      return;
    }
  }

  defaults_ = submitted;
  restartScheduled_ = true;
  restartAtMs_ = millis() + 1000;
  sendPage(200, submitted, "设置已保存，设备正在重启...");
}

void ApConfigPortal::sendPage(
    int statusCode,
    const homedeck::SetupConfig& values,
    const std::string& message) {
  const std::string html = homedeck::buildSetupPageHtml(apSsid_, values, message);
  server_.send(statusCode, "text/html; charset=utf-8", html.c_str());
}
