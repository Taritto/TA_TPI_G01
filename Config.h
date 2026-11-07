#pragma once

#include <Arduino.h>
#include <DHT.h>

// Configuración compartida integrada desde las ramas de módulos.
constexpr uint8_t DHT_PIN = 33;
constexpr uint8_t DHT_TIPO = DHT11;
constexpr uint8_t POT_PIN = 32;
constexpr unsigned long INTERVALO_DHT_MS = 2000UL;
constexpr unsigned long INTERVALO_POT_MS = 100UL;
constexpr int LED_VERDE_PIN = 23;
constexpr int LED_AZUL_PIN = 2;
constexpr float UMBRAL_DEFECTO_V = 0.5f;
constexpr float UMBRAL_MIN_V = 1.0f;
constexpr float UMBRAL_MAX_V = 1.5f;
constexpr int OLED_SDA = 21;
constexpr int OLED_SCL = 22;
constexpr int OLED_ANCHO = 128;
constexpr int OLED_ALTO = 64;
constexpr unsigned char OLED_DIRECCION = 0x3C;
