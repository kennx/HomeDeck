#include "config/config_store.h"

#include "config/config_validator.h"

namespace homedeck {
namespace {

constexpr const char* kNamespace = "homedeck";
constexpr const char* kWifiSsid = "wifi_ssid";
constexpr const char* kWifiPassword = "wifi_pass";
constexpr const char* kTimezoneIana = "tz";
constexpr const char* kAutoRtc = "auto_rtc";
constexpr const char* kNtpServer = "ntp";
constexpr const char* kConfigured = "configured";
constexpr const char* kForceConfig = "force_cfg";
constexpr const char* kLatitude = "lat";
constexpr const char* kLongitude = "lon";
constexpr const char* kWebcalUrl = "webcal_url";

}  // namespace

bool ConfigStore::begin() {
  started_ = prefs_.begin(kNamespace, false);
  return started_;
}

SetupConfig ConfigStore::loadSetupConfig() const {
  SetupConfig config{};
  config.wifiSsid = prefs_.getString(kWifiSsid, "").c_str();
  config.wifiPassword = prefs_.getString(kWifiPassword, "").c_str();
  config.timezoneIana = prefs_.getString(kTimezoneIana, "Asia/Shanghai").c_str();
  config.autoRtcCorrection = prefs_.getBool(kAutoRtc, false);
  config.ntpServer = prefs_.getString(kNtpServer, "pool.ntp.org").c_str();
  config.latitude = prefs_.getString(kLatitude, kDefaultLatitude).c_str();
  config.longitude = prefs_.getString(kLongitude, kDefaultLongitude).c_str();
  config.webcalUrl = prefs_.getString(kWebcalUrl, "").c_str();

  if (trim(config.latitude).empty()) {
    config.latitude = kDefaultLatitude;
  }
  if (trim(config.longitude).empty()) {
    config.longitude = kDefaultLongitude;
  }
  return config;
}

bool ConfigStore::saveSetupConfig(const SetupConfig& config) {
  if (!started_) {
    return false;
  }
  const bool stringsOk =
      prefs_.putString(kWifiSsid, config.wifiSsid.c_str()) > 0 || config.wifiSsid.empty();
  const bool passwordOk =
      prefs_.putString(kWifiPassword, config.wifiPassword.c_str()) > 0 || config.wifiPassword.empty();
  const bool timezoneOk = prefs_.putString(kTimezoneIana, config.timezoneIana.c_str()) > 0;
  const bool ntpOk = prefs_.putString(kNtpServer, config.ntpServer.c_str()) > 0 || config.ntpServer.empty();
  const bool boolOk = prefs_.putBool(kAutoRtc, config.autoRtcCorrection);
  const bool latOk = prefs_.putString(kLatitude, config.latitude.c_str()) > 0 || config.latitude.empty();
  const bool lonOk = prefs_.putString(kLongitude, config.longitude.c_str()) > 0 || config.longitude.empty();
  const bool webcalOk = prefs_.putString(kWebcalUrl, config.webcalUrl.c_str()) > 0 || config.webcalUrl.empty();
  return stringsOk && passwordOk && timezoneOk && ntpOk && boolOk && latOk && lonOk && webcalOk;
}

BootFlags ConfigStore::loadBootFlags() const {
  return BootFlags{
      prefs_.getBool(kConfigured, false),
      prefs_.getBool(kForceConfig, false),
  };
}

bool ConfigStore::saveConfigured(bool configured) {
  return started_ && prefs_.putBool(kConfigured, configured);
}

bool ConfigStore::setForceConfigOnNextBoot(bool enabled) {
  return started_ && prefs_.putBool(kForceConfig, enabled);
}

bool ConfigStore::clearForceConfigOnNextBoot() {
  return setForceConfigOnNextBoot(false);
}

}  // namespace homedeck
