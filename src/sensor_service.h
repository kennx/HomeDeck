#pragma once

#include <string>

struct SensorSnapshot {
  std::string temperatureText;
  std::string humidityText;
  bool available = false;
};

class SensorService {
 public:
  bool begin();
  SensorSnapshot sample();

 private:
  bool available_ = false;
};
