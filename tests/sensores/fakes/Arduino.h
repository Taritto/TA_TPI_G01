#pragma once
#include <stdint.h>

constexpr uint8_t INPUT = 0;
constexpr uint8_t ADC_11db = 3;

unsigned long millis();
void pinMode(uint8_t pin, uint8_t modo);
void analogReadResolution(uint8_t bits);
void analogSetPinAttenuation(uint8_t pin, uint8_t atenuacion);
uint16_t analogRead(uint8_t pin);
uint32_t analogReadMilliVolts(uint8_t pin);
