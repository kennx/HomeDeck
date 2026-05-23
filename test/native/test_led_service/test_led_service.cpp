#include <unity.h>
#include <Arduino.h>
#include <M5Unified.h>

FakeM5Global M5;

#include "../../../src/led_service.h"
#include "../../../src/led_service.cpp"

void setUp() {
  M5.Led = FakeLed{};
  M5.Power = FakePower{};
  resetLedServicePixelStateForTest();
}

void tearDown() {}

void test_led_service_init() {
  LedService service;
  TEST_ASSERT_TRUE(service.begin());
  const LedServicePixelState& pixels = ledServicePixelStateForTest();
  TEST_ASSERT_TRUE(pixels.begun);
  TEST_ASSERT_EQUAL_UINT8(0, pixels.brightness);
  TEST_ASSERT_EQUAL(1, pixels.showCalls);
}

void test_led_service_orange_when_charging_less_than_100() {
  M5.Power.vbus_voltage = 5000;
  M5.Power.battery_level = 80;

  LedService service;
  service.begin();
  service.update();

  const LedServicePixelState& pixels = ledServicePixelStateForTest();
  TEST_ASSERT_EQUAL_UINT8(60, pixels.brightness);
  TEST_ASSERT_EQUAL_UINT8(255, pixels.r[0]);
  TEST_ASSERT_EQUAL_UINT8(128, pixels.g[0]);
  TEST_ASSERT_EQUAL_UINT8(0, pixels.b[0]);
  TEST_ASSERT_EQUAL_UINT8(255, pixels.r[1]);
  TEST_ASSERT_EQUAL_UINT8(128, pixels.g[1]);
  TEST_ASSERT_EQUAL_UINT8(0, pixels.b[1]);
  TEST_ASSERT_TRUE(pixels.showCalls >= 2);
  TEST_ASSERT_FALSE(M5.Led.displayed);
}

void test_led_service_green_when_charging_at_100() {
  M5.Power.vbus_voltage = 5000;
  M5.Power.battery_level = 100;

  LedService service;
  service.begin();
  service.update();

  const LedServicePixelState& pixels = ledServicePixelStateForTest();
  TEST_ASSERT_EQUAL_UINT8(60, pixels.brightness);
  TEST_ASSERT_EQUAL_UINT8(0, pixels.r[0]);
  TEST_ASSERT_EQUAL_UINT8(255, pixels.g[0]);
  TEST_ASSERT_EQUAL_UINT8(0, pixels.b[0]);
  TEST_ASSERT_EQUAL_UINT8(0, pixels.r[1]);
  TEST_ASSERT_EQUAL_UINT8(255, pixels.g[1]);
  TEST_ASSERT_EQUAL_UINT8(0, pixels.b[1]);
}

void test_led_service_blinking_red_when_low_battery() {
  M5.Power.vbus_voltage = 0;
  M5.Power.battery_level = 3;

  LedService service;
  service.begin();
  service.update();

  const LedServicePixelState& pixels = ledServicePixelStateForTest();
  TEST_ASSERT_TRUE(pixels.brightness >= 10 && pixels.brightness <= 90);
  TEST_ASSERT_EQUAL_UINT8(255, pixels.r[0]);
  TEST_ASSERT_EQUAL_UINT8(0, pixels.g[0]);
  TEST_ASSERT_EQUAL_UINT8(0, pixels.b[0]);
  TEST_ASSERT_EQUAL_UINT8(255, pixels.r[1]);
  TEST_ASSERT_EQUAL_UINT8(0, pixels.g[1]);
  TEST_ASSERT_EQUAL_UINT8(0, pixels.b[1]);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_led_service_init);
  RUN_TEST(test_led_service_orange_when_charging_less_than_100);
  RUN_TEST(test_led_service_green_when_charging_at_100);
  RUN_TEST(test_led_service_blinking_red_when_low_battery);
  return UNITY_END();
}
