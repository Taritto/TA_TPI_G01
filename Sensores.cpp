#include "Sensores.h"

#include <DHT.h>

#include "Config.h"

namespace {

DHT dht(DHT_PIN, DHT_TIPO);
Mediciones mediciones;
unsigned long ultimoIntentoDhtMs = 0;
unsigned long ultimoIntentoPotMs = 0;
bool primeraLecturaPot = true;

static_assert(INTERVALO_DHT_MS >= 2000UL,
              "El DHT22 requiere al menos 2000 ms entre lecturas");
static_assert(INTERVALO_POT_MS > 0UL,
              "El intervalo del potenciometro debe ser positivo");

void leerDht(unsigned long ahoraMs) {
  ultimoIntentoDhtMs = ahoraMs;
  mediciones.ultimaLecturaDhtMs = ahoraMs;

  const float temperatura = dht.readTemperature();
  const float humedad = dht.readHumidity();
  mediciones.dhtValido = isfinite(temperatura) && isfinite(humedad);
  // El par es una sola medicion: si falla una parte, invalidar ambas.
  mediciones.temperaturaC = mediciones.dhtValido ? temperatura : NAN;
  mediciones.humedadPorcentaje = mediciones.dhtValido ? humedad : NAN;
}

void leerPotenciometro(unsigned long ahoraMs) {
  primeraLecturaPot = false;
  ultimoIntentoPotMs = ahoraMs;
  mediciones.ultimaLecturaPotMs = ahoraMs;

  const int lecturaADC = analogRead(POT_PIN);
  // Son dos conversiones consecutivas, igual que en la base; no representan
  // necesariamente la misma muestra si el potenciometro se esta moviendo.
  const uint32_t milivoltios = analogReadMilliVolts(POT_PIN);
  mediciones.potenciometroADC = lecturaADC;
  mediciones.potenciometroValido =
      lecturaADC >= 0 && lecturaADC <= 4095 && milivoltios <= 3300UL;
  mediciones.potenciometroV = mediciones.potenciometroValido
                                ? milivoltios / 1000.0f
                                : NAN;
}

}  // namespace

namespace Sensores {

void iniciar() {
  mediciones = Mediciones{};
  primeraLecturaPot = true;
  pinMode(POT_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);
  dht.begin();
  // Esperar un intervalo desde el inicio antes de la primera lectura DHT.
  ultimoIntentoDhtMs = millis();
  ultimoIntentoPotMs = ultimoIntentoDhtMs;
}

void actualizar(unsigned long ahoraMs) {
  if (primeraLecturaPot ||
      ahoraMs - ultimoIntentoPotMs >= INTERVALO_POT_MS) {
    leerPotenciometro(ahoraMs);
  }
  // La resta unsigned sigue funcionando cuando millis() desborda.
  if (ahoraMs - ultimoIntentoDhtMs >= INTERVALO_DHT_MS) {
    leerDht(ahoraMs);
  }
}

Mediciones obtenerMediciones() {
  return mediciones;
}

}  // namespace Sensores
