#include <Arduino.h>

#include "boot_controller.h"

namespace {

BootController gBootController;

}  // namespace

void setup() {
  gBootController.begin();
}

void loop() {
  gBootController.update();
  gBootController.enterDeepSleep();
}
