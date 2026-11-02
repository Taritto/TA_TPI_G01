#pragma once

#include <Arduino.h>
#include <math.h>

// Instantanea del ultimo intento de lectura de cada dispositivo.
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
