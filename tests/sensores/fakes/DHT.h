#pragma once
#include <stdint.h>

constexpr uint8_t DHT11 = 11;
constexpr uint8_t DHT22 = 22;

class DHT {
 public:
  DHT(uint8_t pin, uint8_t tipo);
  void begin();
  float readTemperature();
  float readHumidity();
};
