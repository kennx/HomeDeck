#include <unity.h>

#include "config/setup_page.h"

void test_select_top_five_wifi_networks_by_rssi() {
  std::vector<homedeck::WifiNetwork> networks = {
      {"weak", -90},
      {"best", -20},
      {"mid", -60},
      {"ok", -50},
      {"fine", -40},
      {"last", -70},
  };

  const auto selected = homedeck::selectTopWifiNetworks(networks, 5);

  TEST_ASSERT_EQUAL(5, static_cast<int>(selected.size()));
  TEST_ASSERT_EQUAL_STRING("best", selected[0].ssid.c_str());
  TEST_ASSERT_EQUAL_STRING("fine", selected[1].ssid.c_str());
  TEST_ASSERT_EQUAL_STRING("ok", selected[2].ssid.c_str());
  TEST_ASSERT_EQUAL_STRING("mid", selected[3].ssid.c_str());
  TEST_ASSERT_EQUAL_STRING("last", selected[4].ssid.c_str());
}

void test_setup_page_contains_wifi_list_timezone_and_disabled_auto_when_ssid_empty() {
  homedeck::SetupConfig config{};
  config.timezoneIana = "Asia/Shanghai";
  config.ntpServer = "pool.ntp.org";
  std::vector<homedeck::WifiNetwork> networks = {{"Home", -30}, {"Cafe", -50}};

  const std::string html = homedeck::buildSetupPageHtml("HomeDeck-ABCD", config, networks, "");

  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("HomeDeck-ABCD"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("Home"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("Cafe"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("Asia/Shanghai"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("name=\"auto_rtc\""));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("disabled"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("name=\"manual_datetime\""));
}

void test_setup_page_shows_error_message() {
  homedeck::SetupConfig config{};
  std::vector<homedeck::WifiNetwork> networks{};

  const std::string html = homedeck::buildSetupPageHtml("HomeDeck-ABCD", config, networks, "请填写手动时间。");

  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("请填写手动时间。"));
}

void test_setup_page_does_not_embed_ssid_in_inline_javascript() {
  homedeck::SetupConfig config{};
  std::vector<homedeck::WifiNetwork> networks = {{"Bob's WiFi", -30}};

  const std::string html = homedeck::buildSetupPageHtml("HomeDeck-ABCD", config, networks, "");

  TEST_ASSERT_EQUAL(std::string::npos, html.find("onclick="));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("data-ssid=\"Bob&#39;s WiFi\""));
}

void test_setup_page_contains_location_fields() {
  homedeck::SetupConfig config{};
  config.timezoneIana = "Asia/Shanghai";
  config.ntpServer = "pool.ntp.org";
  config.latitude = "31.2304";
  config.longitude = "121.4737";
  std::vector<homedeck::WifiNetwork> networks{};

  const std::string html = homedeck::buildSetupPageHtml("HomeDeck-ABCD", config, networks, "");

  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("name=\"latitude\""));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("name=\"longitude\""));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("31.2304"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("121.4737"));
  TEST_ASSERT_EQUAL(std::string::npos, html.find("navigator.geolocation"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("id=\"osm_link\""));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("class=\"hint\""));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("openstreetmap.org"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, html.find("match(/#map=[0-9.]+"));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_select_top_five_wifi_networks_by_rssi);
  RUN_TEST(test_setup_page_contains_wifi_list_timezone_and_disabled_auto_when_ssid_empty);
  RUN_TEST(test_setup_page_shows_error_message);
  RUN_TEST(test_setup_page_does_not_embed_ssid_in_inline_javascript);
  RUN_TEST(test_setup_page_contains_location_fields);
  return UNITY_END();
}
