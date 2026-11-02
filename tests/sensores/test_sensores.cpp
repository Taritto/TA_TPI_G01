#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

#include "Config.h"
#include "Sensores.h"

namespace {
unsigned long relojMs = 0;
unsigned int lecturasDht = 0;
unsigned int lecturasAdc = 0;
unsigned int lecturasMv = 0;
float temperatura = 24.5f;
float humedad = 60.0f;
uint16_t adc = 2048;
uint32_t mv = 1650;

void comprobarVoltaje(float esperado) {
  const Mediciones m = Sensores::obtenerMediciones();
  assert(m.potenciometroValido);
  assert(fabs(m.potenciometroV - esperado) < 0.0001f);
}
}  // namespace

unsigned long millis() { return relojMs; }
void pinMode(uint8_t pin, uint8_t modo) {
  assert(pin == 32 && modo == INPUT);
}
void analogReadResolution(uint8_t bits) { assert(bits == 12); }
void analogSetPinAttenuation(uint8_t pin, uint8_t atenuacion) {
  assert(pin == 32 && atenuacion == ADC_11db);
}
uint16_t analogRead(uint8_t pin) {
  assert(pin == 32);
  ++lecturasAdc;
  return adc;
}
uint32_t analogReadMilliVolts(uint8_t pin) {
  assert(pin == 32);
  ++lecturasMv;
  return mv;
}
DHT::DHT(uint8_t pin, uint8_t tipo) { assert(pin == 33 && tipo == DHT22); }
void DHT::begin() {}
float DHT::readTemperature() { ++lecturasDht; return temperatura; }
float DHT::readHumidity() { return humedad; }

int main() {
  Sensores::iniciar();
  Mediciones m = Sensores::obtenerMediciones();
  assert(!m.dhtValido && !m.potenciometroValido);
  assert(isnan(m.temperaturaC) && isnan(m.potenciometroV));

  Sensores::actualizar(0);
  comprobarVoltaje(1.65f);
  assert(lecturasAdc == 1 && lecturasMv == 1 && lecturasDht == 0);
  Sensores::actualizar(99);
  assert(lecturasAdc == 1);
  Sensores::actualizar(100);
  assert(lecturasAdc == 2 && lecturasDht == 0);
  Sensores::actualizar(1999);
  assert(lecturasDht == 0);
  Sensores::actualizar(2000);
  m = Sensores::obtenerMediciones();
  assert(lecturasDht == 1 && m.dhtValido);
  assert(m.temperaturaC == 24.5f && m.humedadPorcentaje == 60.0f);
  assert(m.ultimaLecturaDhtMs == 2000);

  // La consulta devuelve una copia y no vuelve a leer los dispositivos.
  const unsigned int prevAdc = lecturasAdc;
  m.temperaturaC = 999.0f;
  assert(Sensores::obtenerMediciones().temperaturaC == 24.5f);
  Sensores::obtenerMediciones();
  assert(lecturasDht == 1 && lecturasAdc == prevAdc);

  temperatura = NAN;
  Sensores::actualizar(4000);
  m = Sensores::obtenerMediciones();
  assert(!m.dhtValido && isnan(m.temperaturaC) && isnan(m.humedadPorcentaje));
  assert(m.potenciometroValido && m.ultimaLecturaDhtMs == 4000);
  temperatura = 25.0f;
  humedad = INFINITY;
  Sensores::actualizar(6000);
  assert(!Sensores::obtenerMediciones().dhtValido);
  humedad = 0.0f;
  temperatura = 0.0f;
  Sensores::actualizar(8000);
  assert(Sensores::obtenerMediciones().dhtValido);

  adc = 0; mv = 0;
  Sensores::actualizar(8100);
  comprobarVoltaje(0.0f);
  adc = 4095; mv = 3300;
  Sensores::actualizar(8200);
  comprobarVoltaje(3.3f);
  mv = 3301;
  Sensores::actualizar(8300);
  m = Sensores::obtenerMediciones();
  assert(!m.potenciometroValido && isnan(m.potenciometroV));
  mv = 1000; adc = 4096;
  Sensores::actualizar(8400);
  assert(!Sensores::obtenerMediciones().potenciometroValido);
  adc = 1240;
  Sensores::actualizar(8500);
  comprobarVoltaje(1.0f);

  // iniciar() descarta datos y reinicia la espera desde el reloj actual.
  relojMs = ULONG_MAX - 1000UL;
  Sensores::iniciar();
  assert(!Sensores::obtenerMediciones().dhtValido);
  const unsigned int prevDht = lecturasDht;
  Sensores::actualizar(relojMs);
  comprobarVoltaje(1.0f);
  Sensores::actualizar(relojMs + 1999UL);
  assert(lecturasDht == prevDht);
  Sensores::actualizar(relojMs + 2000UL);
  assert(lecturasDht == prevDht + 1);
  assert(Sensores::obtenerMediciones().dhtValido);
  assert(Sensores::obtenerMediciones().ultimaLecturaDhtMs == relojMs + 2000UL);

  puts("OK: intervalos, arranque, copias, errores, extremos, recuperacion y rollover");
}
