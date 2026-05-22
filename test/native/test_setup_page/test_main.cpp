#include <string>

#include <unity.h>

#include "homedeck/config_types.h"
#include "homedeck/setup_page.h"

using homedeck::SetupConfig;
using homedeck::buildSetupPageHtml;
using homedeck::buildWifiQrPayload;

void test_builds_wifi_qr_payload_for_open_ap() {
  TEST_ASSERT_EQUAL_STRING(
      "WIFI:T:nopass;S:HomeDeck Setup;;",
      buildWifiQrPayload("HomeDeck Setup").c_str());
}

void test_builds_setup_page_with_required_fields_and_default_ntp() {
  const SetupConfig defaults{
      "",
      "",
      "Etc/UTC",
      "",
      "",
      "",
  };

  const std::string html = buildSetupPageHtml("HomeDeck Setup", defaults, "");

  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("<html lang=\"zh-CN\">"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("Wi-Fi 名称"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("Wi-Fi 密码"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("时区"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("NTP 服务器"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("个人日程 webcal 地址"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("节假日 webcal 地址"));
  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      html.find("<input id=\"ntp_server\" name=\"ntp_server\" value=\"pool.ntp.org\""));
}

void test_builds_setup_page_with_calendar_values_and_chinese_message() {
  const SetupConfig defaults{
      "LivingRoomWiFi",
      "secret",
      "Asia/Shanghai",
      "time.cloudflare.com",
      "webcal://example.com/personal.ics",
      "https://example.com/holiday.ics",
  };

  const std::string html = buildSetupPageHtml(
      "HomeDeck Setup",
      defaults,
      "设置已保存，设备正在重启...");

  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("value=\"Asia/Shanghai\""));
  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      html.find("<input id=\"personal_calendar_url\" name=\"personal_calendar_url\" value=\"webcal://example.com/personal.ics\""));
  TEST_ASSERT_NOT_EQUAL(
      std::string::npos,
      html.find("<input id=\"holiday_calendar_url\" name=\"holiday_calendar_url\" value=\"https://example.com/holiday.ics\""));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("设置已保存，设备正在重启..."));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("保存并重启"));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_builds_wifi_qr_payload_for_open_ap);
  RUN_TEST(test_builds_setup_page_with_required_fields_and_default_ntp);
  RUN_TEST(test_builds_setup_page_with_calendar_values_and_chinese_message);
  return UNITY_END();
}
