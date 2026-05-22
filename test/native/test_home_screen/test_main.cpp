#include <unity.h>

#include "homedeck/home_screen.h"

using homedeck::HomeViewModel;
using homedeck::buildStatusText;

namespace {

HomeViewModel makeHealthyViewModel() {
  HomeViewModel model;
  model.wifiConnected = true;
  model.timeSynced = true;
  model.calendarFresh = true;
  model.sensorAvailable = true;
  return model;
}

}  // namespace

void test_status_offline_has_highest_priority() {
  HomeViewModel model = makeHealthyViewModel();
  model.wifiConnected = false;

  TEST_ASSERT_EQUAL_STRING("离线", buildStatusText(model).c_str());
}

void test_status_time_not_synced_has_priority_over_calendar_and_sensor() {
  HomeViewModel model = makeHealthyViewModel();
  model.timeSynced = false;
  model.calendarFresh = false;
  model.sensorAvailable = false;

  TEST_ASSERT_EQUAL_STRING("时间未同步", buildStatusText(model).c_str());
}

void test_status_calendar_stale_has_priority_over_sensor_unavailable() {
  HomeViewModel model = makeHealthyViewModel();
  model.calendarFresh = false;
  model.sensorAvailable = false;

  TEST_ASSERT_EQUAL_STRING("日历未更新", buildStatusText(model).c_str());
}

void test_status_sensor_unavailable_when_other_inputs_are_ok() {
  HomeViewModel model = makeHealthyViewModel();
  model.sensorAvailable = false;

  TEST_ASSERT_EQUAL_STRING("传感器不可用", buildStatusText(model).c_str());
}

void test_status_updated_when_all_inputs_are_healthy() {
  const HomeViewModel model = makeHealthyViewModel();

  TEST_ASSERT_EQUAL_STRING("已更新", buildStatusText(model).c_str());
}

void test_home_view_model_sensor_is_available_by_default() {
  const HomeViewModel model;

  TEST_ASSERT_TRUE(model.sensorAvailable);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_status_offline_has_highest_priority);
  RUN_TEST(test_status_time_not_synced_has_priority_over_calendar_and_sensor);
  RUN_TEST(test_status_calendar_stale_has_priority_over_sensor_unavailable);
  RUN_TEST(test_status_sensor_unavailable_when_other_inputs_are_ok);
  RUN_TEST(test_status_updated_when_all_inputs_are_healthy);
  RUN_TEST(test_home_view_model_sensor_is_available_by_default);
  return UNITY_END();
}
