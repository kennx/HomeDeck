#include <unity.h>
#include <Arduino.h>
#include <M5Unified.h>

FakeM5Global M5;

#include "../../../src/led_service.h"
#include "../../../src/led_service.cpp"

void setUp() {
  M5.Led = FakeLed{};
  M5.Power = FakePower{};
}

void tearDown() {}

void test_led_service_init() {
  LedService service;
  TEST_ASSERT_TRUE(service.begin());
  TEST_ASSERT_EQUAL_UINT8(0, M5.Led.brightness);
}

void test_led_service_orange_when_charging_less_than_100() {
  M5.Power.vbus_voltage = 5000;
  M5.Power.battery_level = 80;

  LedService service;
  service.begin();
  service.update();

  TEST_ASSERT_EQUAL_UINT8(60, M5.Led.brightness);
  TEST_ASSERT_EQUAL_UINT8(255, M5.Led.r);
  TEST_ASSERT_EQUAL_UINT8(128, M5.Led.g);
  TEST_ASSERT_EQUAL_UINT8(0, M5.Led.b);
}

void test_led_service_green_when_charging_at_100() {
  M5.Power.vbus_voltage = 5000;
  M5.Power.battery_level = 100;

  LedService service;
  service.begin();
  service.update();

  TEST_ASSERT_EQUAL_UINT8(60, M5.Led.brightness);
  TEST_ASSERT_EQUAL_UINT8(0, M5.Led.r);
  TEST_ASSERT_EQUAL_UINT8(255, M5.Led.g);
  TEST_ASSERT_EQUAL_UINT8(0, M5.Led.b);
}

void test_led_service_blinking_red_when_low_battery() {
  M5.Power.vbus_voltage = 0;
  M5.Power.battery_level = 3;

  LedService service;
  service.begin();
  service.update();

  TEST_ASSERT_TRUE(M5.Led.brightness >= 10 && M5.Led.brightness <= 90);
  TEST_ASSERT_EQUAL_UINT8(255, M5.Led.r);
  TEST_ASSERT_EQUAL_UINT8(0, M5.Led.g);
  TEST_ASSERT_EQUAL_UINT8(0, M5.Led.b);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_led_service_init);
  RUN_TEST(test_led_service_orange_when_charging_less_than_100);
  RUN_TEST(test_led_service_green_when_charging_at_100);
  RUN_TEST(test_led_service_blinking_red_when_low_battery);
  return UNITY_END();
}
