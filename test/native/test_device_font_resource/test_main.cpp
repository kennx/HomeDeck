#include <unity.h>

#include "../../../src/generated/device_font_vlw.h"
// Native PlatformIO tests do not link src/generated objects automatically.
#include "../../../src/generated/device_font_vlw.cpp"

namespace {

std::uint32_t read_be32(std::size_t offset) {
  const std::uint8_t* data = homedeck::generated::kDeviceFontVlw + offset;
  return (static_cast<std::uint32_t>(data[0]) << 24) |
         (static_cast<std::uint32_t>(data[1]) << 16) |
         (static_cast<std::uint32_t>(data[2]) << 8) |
         static_cast<std::uint32_t>(data[3]);
}

}  // namespace

void test_generated_device_font_metadata_is_plausible() {
  TEST_ASSERT_GREATER_THAN_UINT32(6000,
                                  homedeck::generated::kDeviceFontGlyphCount);
  TEST_ASSERT_GREATER_THAN_UINT32(100000,
                                  homedeck::generated::kDeviceFontVlwSize);
  TEST_ASSERT_EQUAL_UINT32(14, homedeck::generated::kDeviceFontPixelSize);
}

void test_generated_device_font_header_matches_metadata() {
  TEST_ASSERT_NOT_NULL(homedeck::generated::kDeviceFontVlw);
  TEST_ASSERT_EQUAL_UINT32(homedeck::generated::kDeviceFontGlyphCount,
                           read_be32(0));
  TEST_ASSERT_EQUAL_UINT32(11, read_be32(4));
  TEST_ASSERT_GREATER_THAN_UINT32(0, read_be32(8));
  TEST_ASSERT_EQUAL_UINT32(0, read_be32(12));
  TEST_ASSERT_GREATER_THAN_UINT32(0, read_be32(16));
  TEST_ASSERT_GREATER_THAN_UINT32(0, read_be32(20));
  TEST_ASSERT_GREATER_THAN_UINT32(
      24 + homedeck::generated::kDeviceFontGlyphCount * 28,
      homedeck::generated::kDeviceFontVlwSize);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_generated_device_font_metadata_is_plausible);
  RUN_TEST(test_generated_device_font_header_matches_metadata);
  return UNITY_END();
}
