#include <unity.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "homedeck/boot_mode.h"

#include "../../../src/boot_controller.h"
#include "../../../src/boot_controller.cpp"

namespace {

struct FakeRuntime {
  unsigned long nowMs = 0;
  bool forceButtons = false;
  bool wifiConnectResult = false;
  bool syncTimeResult = false;
  bool fetchSucceeds = false;
  bool fetchReturnsInvalidButOk = false;
  bool sensorAvailable = true;
  int m5BeginCalls = 0;
  int m5UpdateCalls = 0;
  int portalBeginCalls = 0;
  int portalHandleCalls = 0;
  int homeRenderCalls = 0;
  int setupRenderCalls = 0;
  int connectWifiCalls = 0;
  int timeBeginCalls = 0;
  int sensorBeginCalls = 0;
  int sensorSampleCalls = 0;
  int syncCalls = 0;
  int savePersonalCacheCalls = 0;
  int saveHolidayCacheCalls = 0;
  std::string lastApSsid;
  std::string lastTimezonePosix;
  homedeck::HomeViewModel lastModel;
  homedeck::SetupConfig config;
  BootCalendarCacheRecord personalCache;
  BootCalendarCacheRecord holidayCache;
  std::vector<std::pair<unsigned long, TimeSnapshot>> snapshots;
  std::string fetchedPersonalUrl;
  std::string fetchedHolidayUrl;
};

TimeSnapshot snapshotAt(
    const FakeRuntime& runtime,
    unsigned long nowMs,
    const TimeSnapshot& fallback = TimeSnapshot{}) {
  for (const auto& entry : runtime.snapshots) {
    if (entry.first == nowMs) {
      return entry.second;
    }
  }

  return fallback;
}

BootControllerDeps makeDeps(FakeRuntime& runtime) {
  BootControllerDeps deps;
  deps.m5Begin = [&runtime]() { ++runtime.m5BeginCalls; };
  deps.m5Update = [&runtime]() { ++runtime.m5UpdateCalls; };
  deps.areSetupButtonsPressed = [&runtime]() { return runtime.forceButtons; };
  deps.millis = [&runtime]() { return runtime.nowMs; };
  deps.makeAccessPointSuffix = []() { return std::string("ABC123"); };
  deps.connectWifi = [&runtime](const std::string& ssid, const std::string&) {
    ++runtime.connectWifiCalls;
    return !ssid.empty() && runtime.wifiConnectResult;
  };
  deps.loadSetupConfig = [&runtime]() { return runtime.config; };
  deps.saveSetupConfig = [&runtime](const homedeck::SetupConfig& config) {
    runtime.config = config;
    return true;
  };
  deps.loadPersonalCalendarCache = [&runtime]() { return runtime.personalCache; };
  deps.loadHolidayCalendarCache = [&runtime]() { return runtime.holidayCache; };
  deps.savePersonalCalendarCache = [&runtime](const BootCalendarCacheRecord& record) {
    ++runtime.savePersonalCacheCalls;
    runtime.personalCache = record;
    return true;
  };
  deps.saveHolidayCalendarCache = [&runtime](const BootCalendarCacheRecord& record) {
    ++runtime.saveHolidayCacheCalls;
    runtime.holidayCache = record;
    return true;
  };
  deps.beginSetupPortal = [&runtime](
                               const std::string& apSsid,
                               const homedeck::SetupConfig&,
                               const BootControllerSaveCallback&) {
    ++runtime.portalBeginCalls;
    runtime.lastApSsid = apSsid;
  };
  deps.handleSetupPortalClient = [&runtime]() { ++runtime.portalHandleCalls; };
  deps.renderSetupScreen = [&runtime](const std::string& apSsid, const std::string&) {
    ++runtime.setupRenderCalls;
    runtime.lastApSsid = apSsid;
  };
  deps.timeBegin = [&runtime](const char* timezonePosix, const char*) {
    ++runtime.timeBeginCalls;
    runtime.lastTimezonePosix = timezonePosix != nullptr ? timezonePosix : "";
    return true;
  };
  deps.timeSnapshot = [&runtime]() {
    return snapshotAt(runtime, runtime.nowMs, TimeSnapshot{});
  };
  deps.syncTimeFromNtp = [&runtime]() {
    ++runtime.syncCalls;
    return runtime.syncTimeResult;
  };
  deps.sensorBegin = [&runtime]() {
    ++runtime.sensorBeginCalls;
    return true;
  };
  deps.sensorSample = [&runtime]() {
    ++runtime.sensorSampleCalls;
    return runtime.sensorAvailable ? SensorSnapshot{"23.7°C", "56%", true}
                                   : SensorSnapshot{"--", "--", false};
  };
  deps.fetchCalendarText = [&runtime](const std::string& url) {
    if (url == runtime.config.personalCalendarUrl) {
      runtime.fetchedPersonalUrl = url;
    }
    if (url == runtime.config.holidayCalendarUrl) {
      runtime.fetchedHolidayUrl = url;
    }
    if (runtime.fetchReturnsInvalidButOk) {
      return CalendarFetchResult{true, "<html>login</html>", 200, ""};
    }
    if (runtime.fetchSucceeds) {
      if (url == runtime.config.personalCalendarUrl) {
        return CalendarFetchResult{true, "personal-ics", 200, ""};
      }
      if (url == runtime.config.holidayCalendarUrl) {
        return CalendarFetchResult{true, "holiday-ics", 200, ""};
      }
    }
    return CalendarFetchResult{false, "", 503, "offline"};
  };
  deps.parsePersonalCalendar = [](const char*, int, int, int) {
    return ParsedPersonalCalendar{};
  };
  deps.parseHolidayCalendar = [](const char*, int, int, int) {
    return ParsedHolidayCalendar{};
  };
  deps.encodePersonalCalendarCache = [](const ParsedPersonalCalendar&) {
    return std::string();
  };
  deps.decodePersonalCalendarCache = [](const std::string&) {
    return ParsedPersonalCalendar{};
  };
  deps.encodeHolidayCalendarCache = [](const ParsedHolidayCalendar&) {
    return std::string();
  };
  deps.decodeHolidayCalendarCache = [](const std::string&) {
    return ParsedHolidayCalendar{};
  };
  deps.describeLunarDate = [](int, int, int) {
    return LunarDayInfo{"农历 四月初五", "节气 小满"};
  };
  deps.renderHomeScreen = [&runtime](const homedeck::HomeViewModel& model) {
    ++runtime.homeRenderCalls;
    runtime.lastModel = model;
  };
  return deps;
}

void test_begin_enters_ap_mode_when_config_missing() {
  FakeRuntime runtime;
  BootController controller(makeDeps(runtime));

  controller.begin();

  TEST_ASSERT_EQUAL(1, runtime.m5BeginCalls);
  TEST_ASSERT_EQUAL(1, runtime.setupRenderCalls);
  TEST_ASSERT_EQUAL(1, runtime.portalBeginCalls);
  TEST_ASSERT_EQUAL_STRING("HomeDeck-ABC123", runtime.lastApSsid.c_str());
  TEST_ASSERT_EQUAL(0, runtime.homeRenderCalls);
}

void test_begin_enters_home_mode_when_config_present() {
  FakeRuntime runtime;
  runtime.config = {
      "HomeWiFi",
      "secret",
      "Asia/Shanghai",
      "pool.ntp.org",
      "",
      "",
  };
  runtime.snapshots.push_back({
      0,
      TimeSnapshot{"09:30", "2026年5月21日", true, false},
  });

  BootController controller(makeDeps(runtime));
  controller.begin();

  TEST_ASSERT_EQUAL(1, runtime.homeRenderCalls);
  TEST_ASSERT_EQUAL(0, runtime.portalBeginCalls);
  TEST_ASSERT_EQUAL_STRING("未配置个人日程", runtime.lastModel.eventRows[0].titleText.c_str());
  TEST_ASSERT_EQUAL_STRING("未配置节假日", runtime.lastModel.holidayText.c_str());
}

void test_begin_renders_home_before_background_network_work() {
  FakeRuntime runtime;
  runtime.config = {
      "HomeWiFi",
      "secret",
      "Asia/Shanghai",
      "pool.ntp.org",
      "https://calendar.example.com/home.ics",
      "https://calendar.example.com/holiday.ics",
  };
  runtime.wifiConnectResult = true;
  runtime.syncTimeResult = true;
  runtime.fetchSucceeds = true;
  runtime.snapshots.push_back({
      0,
      TimeSnapshot{"09:30", "2026年5月21日", true, false},
  });

  BootController controller(makeDeps(runtime));
  controller.begin();

  TEST_ASSERT_EQUAL(1, runtime.homeRenderCalls);
  TEST_ASSERT_EQUAL(0, runtime.connectWifiCalls);
  TEST_ASSERT_EQUAL(0, runtime.syncCalls);
  TEST_ASSERT_EQUAL_STRING("", runtime.fetchedPersonalUrl.c_str());
  TEST_ASSERT_EQUAL_STRING("", runtime.fetchedHolidayUrl.c_str());
}

void test_begin_maps_iana_timezone_to_posix_for_time_service() {
  FakeRuntime runtime;
  runtime.config = {
      "HomeWiFi",
      "secret",
      "Asia/Shanghai",
      "pool.ntp.org",
      "",
      "",
  };
  runtime.snapshots.push_back({
      0,
      TimeSnapshot{"09:30", "2026年5月21日", true, false},
  });

  BootController controller(makeDeps(runtime));
  controller.begin();

  TEST_ASSERT_EQUAL(1, runtime.timeBeginCalls);
  TEST_ASSERT_EQUAL_STRING("CST-8", runtime.lastTimezonePosix.c_str());
}

void test_home_mode_uses_cache_when_calendar_fetch_fails() {
  FakeRuntime runtime;
  runtime.config = {
      "HomeWiFi",
      "secret",
      "Asia/Shanghai",
      "pool.ntp.org",
      "https://calendar.example.com/home.ics",
      "https://calendar.example.com/holiday.ics",
    };
  runtime.personalCache.updatedAt = "2026年5月21日 08:00";
  runtime.holidayCache.updatedAt = "2026年5月21日 08:00";
  runtime.personalCache.payload = "cached-personal";
  runtime.holidayCache.payload = "cached-holiday";
  runtime.snapshots.push_back({
      0,
      TimeSnapshot{"09:30", "2026年5月21日", true, true},
  });

  BootControllerDeps deps = makeDeps(runtime);
  deps.decodePersonalCalendarCache = [](const std::string& payload) {
    ParsedPersonalCalendar result;
    if (payload == "cached-personal") {
      result.events[0] = {"09:00", "缓存会议"};
      result.eventCount = 1;
    }
    return result;
  };
  deps.decodeHolidayCalendarCache = [](const std::string& payload) {
    if (payload == "cached-holiday") {
      return ParsedHolidayCalendar{"节假日 缓存节日"};
    }
    return ParsedHolidayCalendar{};
  };

  BootController controller(std::move(deps));
  controller.begin();

  TEST_ASSERT_EQUAL_STRING("缓存会议", runtime.lastModel.eventRows[0].titleText.c_str());
  TEST_ASSERT_EQUAL_STRING("节假日 缓存节日", runtime.lastModel.holidayText.c_str());
  TEST_ASSERT_FALSE(runtime.lastModel.calendarFresh);
}

void test_home_mode_ignores_stale_cache_from_previous_day() {
  FakeRuntime runtime;
  runtime.config = {
      "HomeWiFi",
      "secret",
      "Asia/Shanghai",
      "pool.ntp.org",
      "https://calendar.example.com/home.ics",
      "https://calendar.example.com/holiday.ics",
  };
  runtime.personalCache.updatedAt = "2026年5月20日 08:00";
  runtime.holidayCache.updatedAt = "2026年5月20日 08:00";
  runtime.personalCache.payload = "cached-personal";
  runtime.holidayCache.payload = "cached-holiday";
  runtime.snapshots.push_back({
      0,
      TimeSnapshot{"09:30", "2026年5月21日", true, false},
  });

  BootControllerDeps deps = makeDeps(runtime);
  deps.decodePersonalCalendarCache = [](const std::string&) {
    ParsedPersonalCalendar result;
    result.events[0] = {"09:00", "昨天的缓存会议"};
    result.eventCount = 1;
    return result;
  };
  deps.decodeHolidayCalendarCache = [](const std::string&) {
    return ParsedHolidayCalendar{"节假日 昨天的缓存节日"};
  };

  BootController controller(std::move(deps));
  controller.begin();

  TEST_ASSERT_EQUAL_STRING("今日日程不可用", runtime.lastModel.eventRows[0].titleText.c_str());
  TEST_ASSERT_EQUAL_STRING("节假日不可用", runtime.lastModel.holidayText.c_str());
  TEST_ASSERT_FALSE(runtime.lastModel.calendarFresh);
}

void test_update_does_not_fetch_or_overwrite_cache_when_date_is_invalid() {
  FakeRuntime runtime;
  runtime.config = {
      "HomeWiFi",
      "secret",
      "Asia/Shanghai",
      "pool.ntp.org",
      "https://calendar.example.com/home.ics",
      "https://calendar.example.com/holiday.ics",
  };
  runtime.wifiConnectResult = true;
  runtime.fetchSucceeds = true;
  runtime.snapshots.push_back({0, TimeSnapshot{"--:--", "", false, false}});
  runtime.nowMs = 1;
  runtime.snapshots.push_back({1, TimeSnapshot{"--:--", "", false, false}});

  BootController controller(makeDeps(runtime));
  controller.begin();
  controller.update();

  TEST_ASSERT_EQUAL_STRING("", runtime.fetchedPersonalUrl.c_str());
  TEST_ASSERT_EQUAL_STRING("", runtime.fetchedHolidayUrl.c_str());
  TEST_ASSERT_EQUAL(0, runtime.savePersonalCacheCalls);
  TEST_ASSERT_EQUAL(0, runtime.saveHolidayCacheCalls);
}

void test_update_runs_background_sync_and_refreshes_after_success() {
  FakeRuntime runtime;
  runtime.config = {
      "HomeWiFi",
      "secret",
      "Asia/Shanghai",
      "pool.ntp.org",
      "https://calendar.example.com/home.ics",
      "https://calendar.example.com/holiday.ics",
  };
  runtime.wifiConnectResult = true;
  runtime.syncTimeResult = true;
  runtime.fetchSucceeds = true;
  runtime.snapshots.push_back({0, TimeSnapshot{"09:30", "2026年5月21日", true, false}});
  runtime.nowMs = 1;
  runtime.snapshots.push_back({1, TimeSnapshot{"09:31", "2026年5月21日", true, true}});

  BootControllerDeps deps = makeDeps(runtime);
  deps.parsePersonalCalendar = [](const char* ics, int year, int month, int day) {
    TEST_ASSERT_EQUAL_STRING("personal-ics", ics);
    TEST_ASSERT_EQUAL(2026, year);
    TEST_ASSERT_EQUAL(5, month);
    TEST_ASSERT_EQUAL(21, day);
    ParsedPersonalCalendar result;
    result.events[0] = {"12:00", "联网会议"};
    result.eventCount = 1;
    return result;
  };
  deps.parseHolidayCalendar = [](const char* ics, int year, int month, int day) {
    TEST_ASSERT_EQUAL_STRING("holiday-ics", ics);
    TEST_ASSERT_EQUAL(2026, year);
    TEST_ASSERT_EQUAL(5, month);
    TEST_ASSERT_EQUAL(21, day);
    return ParsedHolidayCalendar{"节假日 联网节日"};
  };
  deps.encodePersonalCalendarCache = [](const ParsedPersonalCalendar& calendar) {
    TEST_ASSERT_EQUAL_UINT32(1, calendar.eventCount);
    return std::string("saved-personal");
  };
  deps.encodeHolidayCalendarCache = [](const ParsedHolidayCalendar& calendar) {
    TEST_ASSERT_EQUAL_STRING("节假日 联网节日", calendar.holidayText.c_str());
    return std::string("saved-holiday");
  };

  BootController controller(std::move(deps));
  controller.begin();
  TEST_ASSERT_EQUAL(1, runtime.homeRenderCalls);

  controller.update();

  TEST_ASSERT_EQUAL(1, runtime.connectWifiCalls);
  TEST_ASSERT_EQUAL(1, runtime.syncCalls);
  TEST_ASSERT_EQUAL_STRING("https://calendar.example.com/home.ics", runtime.fetchedPersonalUrl.c_str());
  TEST_ASSERT_EQUAL_STRING("https://calendar.example.com/holiday.ics", runtime.fetchedHolidayUrl.c_str());
  TEST_ASSERT_EQUAL(2, runtime.homeRenderCalls);
  TEST_ASSERT_EQUAL_STRING("联网会议", runtime.lastModel.eventRows[0].titleText.c_str());
  TEST_ASSERT_EQUAL_STRING("节假日 联网节日", runtime.lastModel.holidayText.c_str());
  TEST_ASSERT_TRUE(runtime.lastModel.timeSynced);
  TEST_ASSERT_TRUE(runtime.lastModel.calendarFresh);
}

void test_update_samples_sensor_every_sixty_seconds_and_refreshes_on_recovery() {
  FakeRuntime runtime;
  runtime.config = {
      "HomeWiFi",
      "secret",
      "Asia/Shanghai",
      "pool.ntp.org",
      "",
      "",
  };
  runtime.sensorAvailable = false;
  runtime.snapshots.push_back({0, TimeSnapshot{"09:30", "2026年5月21日", true, false}});
  runtime.nowMs = 59999;
  runtime.snapshots.push_back({59999, TimeSnapshot{"09:30", "2026年5月21日", true, false}});
  runtime.nowMs = 60000;
  runtime.snapshots.push_back({60000, TimeSnapshot{"09:31", "2026年5月21日", true, false}});

  BootController controller(makeDeps(runtime));
  runtime.nowMs = 0;
  controller.begin();
  TEST_ASSERT_EQUAL(1, runtime.homeRenderCalls);
  TEST_ASSERT_FALSE(runtime.lastModel.sensorAvailable);

  runtime.nowMs = 59999;
  controller.update();
  TEST_ASSERT_EQUAL(1, runtime.homeRenderCalls);

  runtime.sensorAvailable = true;
  runtime.nowMs = 60000;
  controller.update();
  TEST_ASSERT_EQUAL(2, runtime.homeRenderCalls);
  TEST_ASSERT_TRUE(runtime.lastModel.sensorAvailable);
}

void test_update_retries_network_before_one_hour_when_first_attempt_failed() {
  FakeRuntime runtime;
  runtime.config = {
      "HomeWiFi",
      "secret",
      "Asia/Shanghai",
      "pool.ntp.org",
      "https://calendar.example.com/home.ics",
      "https://calendar.example.com/holiday.ics",
  };
  runtime.snapshots.push_back({0, TimeSnapshot{"09:30", "2026年5月21日", true, false}});
  runtime.nowMs = 1;
  runtime.snapshots.push_back({1, TimeSnapshot{"09:31", "2026年5月21日", true, false}});
  runtime.nowMs = 5001;
  runtime.snapshots.push_back({5001, TimeSnapshot{"09:35", "2026年5月21日", true, false}});

  BootController controller(makeDeps(runtime));
  runtime.nowMs = 0;
  controller.begin();
  TEST_ASSERT_EQUAL(0, runtime.connectWifiCalls);

  runtime.nowMs = 1;
  controller.update();
  TEST_ASSERT_EQUAL(1, runtime.connectWifiCalls);

  runtime.wifiConnectResult = true;
  runtime.syncTimeResult = true;
  runtime.fetchSucceeds = true;
  runtime.nowMs = 5001;
  controller.update();

  TEST_ASSERT_EQUAL(2, runtime.connectWifiCalls);
  TEST_ASSERT_EQUAL(1, runtime.syncCalls);
  TEST_ASSERT_EQUAL_STRING("https://calendar.example.com/home.ics", runtime.fetchedPersonalUrl.c_str());
}

void test_invalid_successful_calendar_response_keeps_same_day_cache() {
  FakeRuntime runtime;
  runtime.config = {
      "HomeWiFi",
      "secret",
      "Asia/Shanghai",
      "pool.ntp.org",
      "https://calendar.example.com/home.ics",
      "https://calendar.example.com/holiday.ics",
  };
  runtime.wifiConnectResult = true;
  runtime.fetchReturnsInvalidButOk = true;
  runtime.personalCache.payload = "cached-personal";
  runtime.personalCache.updatedAt = "2026年5月21日 08:00";
  runtime.holidayCache.payload = "cached-holiday";
  runtime.holidayCache.updatedAt = "2026年5月21日 08:00";
  runtime.snapshots.push_back({0, TimeSnapshot{"09:30", "2026年5月21日", true, false}});
  runtime.nowMs = 1;
  runtime.snapshots.push_back({1, TimeSnapshot{"09:31", "2026年5月21日", true, false}});

  BootControllerDeps deps = makeDeps(runtime);
  deps.parsePersonalCalendar = [](const char* ics, int, int, int) {
    TEST_ASSERT_EQUAL_STRING("<html>login</html>", ics);
    return ParsedPersonalCalendar{};
  };
  deps.parseHolidayCalendar = [](const char* ics, int, int, int) {
    TEST_ASSERT_EQUAL_STRING("<html>login</html>", ics);
    return ParsedHolidayCalendar{};
  };
  deps.decodePersonalCalendarCache = [](const std::string& payload) {
    ParsedPersonalCalendar result;
    if (payload == "cached-personal") {
      result.events[0] = {"09:00", "缓存会议"};
      result.eventCount = 1;
    }
    return result;
  };
  deps.decodeHolidayCalendarCache = [](const std::string& payload) {
    if (payload == "cached-holiday") {
      return ParsedHolidayCalendar{"节假日 缓存节日"};
    }
    return ParsedHolidayCalendar{};
  };
  deps.encodePersonalCalendarCache = [](const ParsedPersonalCalendar&) {
    TEST_FAIL_MESSAGE("invalid response should not overwrite personal cache");
    return std::string();
  };
  deps.encodeHolidayCalendarCache = [](const ParsedHolidayCalendar&) {
    TEST_FAIL_MESSAGE("invalid response should not overwrite holiday cache");
    return std::string();
  };

  BootController controller(std::move(deps));
  runtime.nowMs = 0;
  controller.begin();
  runtime.nowMs = 1;
  controller.update();

  TEST_ASSERT_EQUAL_STRING("缓存会议", runtime.lastModel.eventRows[0].titleText.c_str());
  TEST_ASSERT_EQUAL_STRING("节假日 缓存节日", runtime.lastModel.holidayText.c_str());
  TEST_ASSERT_FALSE(runtime.lastModel.calendarFresh);
}

void test_update_refreshes_home_screen_after_one_hour() {
  FakeRuntime runtime;
  runtime.config = {
      "HomeWiFi",
      "secret",
      "Asia/Shanghai",
      "pool.ntp.org",
      "",
      "",
  };
  runtime.snapshots.push_back({
      0,
      TimeSnapshot{"09:30", "2026年5月21日", true, false},
  });
  runtime.snapshots.push_back({
      3599999,
      TimeSnapshot{"10:29", "2026年5月21日", true, false},
  });
  runtime.snapshots.push_back({
      3600000,
      TimeSnapshot{"10:30", "2026年5月21日", true, false},
  });

  BootController controller(makeDeps(runtime));
  controller.begin();
  TEST_ASSERT_EQUAL(1, runtime.homeRenderCalls);

  runtime.nowMs = 3599999;
  controller.update();
  TEST_ASSERT_EQUAL(1, runtime.homeRenderCalls);

  runtime.nowMs = 3600000;
  controller.update();
  TEST_ASSERT_EQUAL(2, runtime.homeRenderCalls);
}

}  // namespace

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_begin_enters_ap_mode_when_config_missing);
  RUN_TEST(test_begin_enters_home_mode_when_config_present);
  RUN_TEST(test_begin_renders_home_before_background_network_work);
  RUN_TEST(test_begin_maps_iana_timezone_to_posix_for_time_service);
  RUN_TEST(test_home_mode_uses_cache_when_calendar_fetch_fails);
  RUN_TEST(test_home_mode_ignores_stale_cache_from_previous_day);
  RUN_TEST(test_update_does_not_fetch_or_overwrite_cache_when_date_is_invalid);
  RUN_TEST(test_update_runs_background_sync_and_refreshes_after_success);
  RUN_TEST(test_update_samples_sensor_every_sixty_seconds_and_refreshes_on_recovery);
  RUN_TEST(test_update_retries_network_before_one_hour_when_first_attempt_failed);
  RUN_TEST(test_invalid_successful_calendar_response_keeps_same_day_cache);
  RUN_TEST(test_update_refreshes_home_screen_after_one_hour);
  return UNITY_END();
}
