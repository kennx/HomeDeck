#include <unity.h>

#include "homedeck/boot_mode.h"

using homedeck::BootInputs;
using homedeck::BootTarget;
using homedeck::decideBootTarget;

void test_unconfigured_enters_access_point_setup() {
  const BootInputs inputs{false, false};

  TEST_ASSERT_EQUAL(
      BootTarget::AccessPointSetup,
      decideBootTarget(inputs));
}

void test_configured_with_force_buttons_enters_access_point_setup() {
  const BootInputs inputs{true, true};

  TEST_ASSERT_EQUAL(
      BootTarget::AccessPointSetup,
      decideBootTarget(inputs));
}

void test_configured_without_force_buttons_enters_connect_and_home() {
  const BootInputs inputs{true, false};

  TEST_ASSERT_EQUAL(
      BootTarget::ConnectAndHome,
      decideBootTarget(inputs));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_unconfigured_enters_access_point_setup);
  RUN_TEST(test_configured_with_force_buttons_enters_access_point_setup);
  RUN_TEST(test_configured_without_force_buttons_enters_connect_and_home);
  return UNITY_END();
}
