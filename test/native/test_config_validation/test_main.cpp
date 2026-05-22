#include <type_traits>

#include <unity.h>

#include "homedeck/config_types.h"
#include "homedeck/config_validator.h"
#include "homedeck/timezone_catalog.h"

using homedeck::SetupConfig;
using homedeck::ValidationError;
using homedeck::findTimezoneByIana;
using homedeck::normalizeCalendarUrl;
using homedeck::validateSetupConfig;
using homedeck::validationMessage;

static_assert(
    std::is_same_v<decltype(validationMessage(ValidationError::None)), const char*>,
    "validationMessage should return const char*");

void test_rejects_empty_wifi_ssid() {
  const SetupConfig config{"", "secret", "Asia/Shanghai", "pool.ntp.org", "", ""};

  TEST_ASSERT_EQUAL(
      ValidationError::MissingWifiSsid,
      validateSetupConfig(config));
  TEST_ASSERT_EQUAL_STRING(
      "Wi-Fi 名称不能为空。",
      validationMessage(ValidationError::MissingWifiSsid));
}

void test_rejects_whitespace_only_wifi_ssid() {
  const SetupConfig config{"   ", "secret", "Asia/Shanghai", "pool.ntp.org", "", ""};

  TEST_ASSERT_EQUAL(
      ValidationError::MissingWifiSsid,
      validateSetupConfig(config));
}

void test_rejects_unknown_timezone() {
  const SetupConfig config{
      "HomeWiFi", "secret", "Mars/Olympus", "pool.ntp.org", "", ""};

  TEST_ASSERT_EQUAL(
      ValidationError::InvalidTimezone,
      validateSetupConfig(config));
  TEST_ASSERT_EQUAL_STRING(
      "时区必须是支持列表中的 IANA 时区。",
      validationMessage(ValidationError::InvalidTimezone));
}

void test_rejects_empty_ntp_server() {
  const SetupConfig config{"HomeWiFi", "secret", "Asia/Shanghai", "", "", ""};

  TEST_ASSERT_EQUAL(
      ValidationError::MissingNtpServer,
      validateSetupConfig(config));
  TEST_ASSERT_EQUAL_STRING(
      "NTP 服务器不能为空。",
      validationMessage(ValidationError::MissingNtpServer));
}

void test_rejects_whitespace_only_ntp_server() {
  const SetupConfig config{"HomeWiFi", "secret", "Asia/Shanghai", "   ", "", ""};

  TEST_ASSERT_EQUAL(
      ValidationError::MissingNtpServer,
      validateSetupConfig(config));
}

void test_allows_empty_calendar_urls() {
  const SetupConfig config{
      "HomeWiFi",
      "secret",
      "Asia/Shanghai",
      "pool.ntp.org",
      "",
      "",
  };

  TEST_ASSERT_EQUAL(ValidationError::None, validateSetupConfig(config));
}

void test_rejects_invalid_personal_calendar_url() {
  const SetupConfig config{
      "HomeWiFi",
      "secret",
      "Asia/Shanghai",
      "pool.ntp.org",
      "ftp://calendar.example.com/feed.ics",
      "",
  };

  TEST_ASSERT_EQUAL(
      ValidationError::InvalidPersonalCalendarUrl,
      validateSetupConfig(config));
  TEST_ASSERT_EQUAL_STRING(
      "个人日程地址必须以 webcal://、http:// 或 https:// 开头。",
      validationMessage(ValidationError::InvalidPersonalCalendarUrl));
}

void test_rejects_invalid_holiday_calendar_url() {
  const SetupConfig config{
      "HomeWiFi",
      "secret",
      "Asia/Shanghai",
      "pool.ntp.org",
      "",
      "calendar.example.com/holiday.ics",
  };

  TEST_ASSERT_EQUAL(
      ValidationError::InvalidHolidayCalendarUrl,
      validateSetupConfig(config));
  TEST_ASSERT_EQUAL_STRING(
      "节假日地址必须以 webcal://、http:// 或 https:// 开头。",
      validationMessage(ValidationError::InvalidHolidayCalendarUrl));
}

void test_normalizes_webcal_calendar_url_to_https() {
  TEST_ASSERT_EQUAL_STRING(
      "https://calendar.example.com/feed.ics",
      normalizeCalendarUrl("webcal://calendar.example.com/feed.ics").c_str());
}

void test_keeps_https_calendar_url_unchanged() {
  TEST_ASSERT_EQUAL_STRING(
      "https://calendar.example.com/feed.ics",
      normalizeCalendarUrl("https://calendar.example.com/feed.ics").c_str());
}

void test_keeps_http_calendar_url_unchanged() {
  TEST_ASSERT_EQUAL_STRING(
      "http://calendar.example.com/feed.ics",
      normalizeCalendarUrl("http://calendar.example.com/feed.ics").c_str());
}

void test_finds_asia_shanghai_timezone_mapping() {
  const auto* timezone = findTimezoneByIana("Asia/Shanghai");

  TEST_ASSERT_NOT_NULL(timezone);
  TEST_ASSERT_EQUAL_STRING("Asia/Shanghai", std::string(timezone->iana).c_str());
  TEST_ASSERT_EQUAL_STRING("CST-8", std::string(timezone->posix).c_str());
}

void test_accepts_valid_setup_config() {
  const SetupConfig config{
      "HomeWiFi",
      "secret",
      "Asia/Shanghai",
      "pool.ntp.org",
      "webcal://calendar.example.com/personal.ics",
      "https://calendar.example.com/holiday.ics",
  };

  TEST_ASSERT_EQUAL(ValidationError::None, validateSetupConfig(config));
  const char* message = validationMessage(ValidationError::None);
  TEST_ASSERT_EQUAL_STRING("", message);
}

void test_accepts_valid_setup_config_with_http_calendar_urls() {
  const SetupConfig config{
      "HomeWiFi",
      "secret",
      "Asia/Shanghai",
      "pool.ntp.org",
      "http://calendar.example.com/personal.ics",
      "http://calendar.example.com/holiday.ics",
  };

  TEST_ASSERT_EQUAL(ValidationError::None, validateSetupConfig(config));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_rejects_empty_wifi_ssid);
  RUN_TEST(test_rejects_whitespace_only_wifi_ssid);
  RUN_TEST(test_rejects_unknown_timezone);
  RUN_TEST(test_rejects_empty_ntp_server);
  RUN_TEST(test_rejects_whitespace_only_ntp_server);
  RUN_TEST(test_allows_empty_calendar_urls);
  RUN_TEST(test_rejects_invalid_personal_calendar_url);
  RUN_TEST(test_rejects_invalid_holiday_calendar_url);
  RUN_TEST(test_normalizes_webcal_calendar_url_to_https);
  RUN_TEST(test_keeps_https_calendar_url_unchanged);
  RUN_TEST(test_keeps_http_calendar_url_unchanged);
  RUN_TEST(test_finds_asia_shanghai_timezone_mapping);
  RUN_TEST(test_accepts_valid_setup_config);
  RUN_TEST(test_accepts_valid_setup_config_with_http_calendar_urls);
  return UNITY_END();
}
