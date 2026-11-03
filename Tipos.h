#pragma once
#include <Arduino.h>
#include <math.h>

struct Mediciones {
  float temperaturaC = NAN;
  float humedadPorcentaje = NAN;
  int potenciometroADC = 0;
  float potenciometroV = NAN;
  bool dhtValido = false;
  bool potenciometroValido = false;
  unsigned long ultimaLecturaDhtMs = 0;
  unsigned long ultimaLecturaPotMs = 0;
};

struct EstadoActuadores {
  bool ledVerde = false;
  bool ledAzul = false;
  bool automatico = false;
  float umbralV = 0.5f;
  float lineaBaseV = 0.0f;
};

enum class VistaPantalla { Inicio, Leds, Potenciometro, Dht, Error };
