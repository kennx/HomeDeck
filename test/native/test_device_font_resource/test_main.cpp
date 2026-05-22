#include <unity.h>

#include "../../../src/generated/device_font_vlw.h"

void test_generated_device_font_metadata_is_plausible() {
  TEST_ASSERT_GREATER_THAN_UINT32(6000,
                                  homedeck::generated::kDeviceFontGlyphCount);
  TEST_ASSERT_GREATER_THAN_UINT32(100000,
                                  homedeck::generated::kDeviceFontVlwSize);
  TEST_ASSERT_EQUAL_UINT32(14, homedeck::generated::kDeviceFontPixelSize);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_generated_device_font_metadata_is_plausible);
  return UNITY_END();
}
