#include <unity.h>

#include <cstdlib>
#include <ctime>

#include "../support/fake_arduino/M5Unified.h"
#include "../support/fake_arduino/Arduino.h"
#include "../support/fake_arduino/esp_sntp.h"

FakeM5Global M5;

namespace {

time_t gFakeNow = 0;

time_t fakeTimeNow(time_t* out) {
  if (out != nullptr) {
    *out = gFakeNow;
  }
  return gFakeNow;
}

int fakeConfigTzTime(const char* tz, const char* server) {
  (void)tz;
  (void)server;
  return 0;
}

void resetFakes() {
  M5 = FakeM5Global{};
  gFakeNow = 0;
  gFakeDelayCallback = nullptr;
  gFakeSntpCallback = nullptr;
  setenv("TZ", "UTC0", 1);
  tzset();
}

}  // namespace

#define ARDUINO 1
#define time(...) fakeTimeNow(__VA_ARGS__)
#define configTzTime(...) fakeConfigTzTime(__VA_ARGS__)
#include "../../../src/time_service.cpp"
#undef configTzTime
#undef time
#undef ARDUINO

namespace {

void test_snapshot_converts_rtc_utc_time_into_local_timezone_in_arduino_build() {
  resetFakes();
  setenv("TZ", "CST-8", 1);
  tzset();

  M5.Rtc.enabled = true;
  M5.Rtc.getDateTimeOk = true;
  M5.Rtc.dateTime = {{2026, 5, 21, 4}, {1, 30, 0}};

  TimeService service;
  TEST_ASSERT_TRUE(service.begin("CST-8", "pool.ntp.org"));

  const TimeSnapshot snapshot = service.snapshot();
  TEST_ASSERT_TRUE(snapshot.timeValid);
  TEST_ASSERT_EQUAL_STRING("09:30", snapshot.timeText.c_str());
  TEST_ASSERT_EQUAL_STRING("2026年5月21日", snapshot.dateText.c_str());
}

}  // namespace

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_snapshot_converts_rtc_utc_time_into_local_timezone_in_arduino_build);
  return UNITY_END();
}
