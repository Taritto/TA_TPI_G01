#pragma once

#include <Arduino.h>
#include <DHT.h>

// Configuracion de Sensores. Los demas modulos agregaran su configuracion
// compartida al integrarse, sin redefinir estos valores en el sketch.
constexpr uint8_t DHT_PIN = 33;
constexpr uint8_t DHT_TIPO = DHT11;
constexpr uint8_t POT_PIN = 32;
constexpr unsigned long INTERVALO_DHT_MS = 2000UL;
constexpr unsigned long INTERVALO_POT_MS = 100UL;
