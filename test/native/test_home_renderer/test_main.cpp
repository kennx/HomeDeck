#include <unity.h>

#include <M5Unified.h>

#include "home_renderer.h"

void setUp() {
  M5 = FakeM5Global{};
}

void tearDown() {
}

void test_home_renderer_draws_centered_home_deck_in_portrait() {
  homedeck::HomeRenderer renderer;

  renderer.render();

  TEST_ASSERT_EQUAL(0, M5.Display.rotation);
  TEST_ASSERT_EQUAL(TFT_WHITE, M5.Display.fillScreenColor);
  TEST_ASSERT_EQUAL(
      static_cast<int>(textdatum_t::middle_center),
      static_cast<int>(M5.Display.textDatum));
  TEST_ASSERT_EQUAL(1, static_cast<int>(M5.Display.prints.size()));
  TEST_ASSERT_EQUAL_STRING("HomeDeck", M5.Display.prints[0].text.c_str());
  TEST_ASSERT_EQUAL(M5.Display.width() / 2, M5.Display.prints[0].x);
  TEST_ASSERT_EQUAL(M5.Display.height() / 2, M5.Display.prints[0].y);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_home_renderer_draws_centered_home_deck_in_portrait);
  return UNITY_END();
}
