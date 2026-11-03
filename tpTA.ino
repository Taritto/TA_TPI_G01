#include "Tipos.h"
#include "Conectividad.h"
#include "Actuadores.h"
#include "Pantalla.h"
#include "Telemetria.h"
#include "Comandos.h"
#include "Telegram.h"

#include "Secretos.h"

const char* WIFI_SSID = WIFI_SSID_LOCAL;
const char* WIFI_PASS = WIFI_PASS_LOCAL;
const char* BOT_TOKEN = BOT_TOKEN_LOCAL;

Mediciones mediciones;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- INICIANDO TEST MODULO TELEGRAM Y COMANDOS ---");

  Actuadores::iniciar();
  Conectividad::iniciar(WIFI_SSID, WIFI_PASS);
  Telegram::iniciar(BOT_TOKEN);

  // Simulamos mediciones válidas para probar
  mediciones.temperaturaC = 24.5f;
  mediciones.humedadPorcentaje = 60.0f;
  mediciones.potenciometroADC = 1860;
  mediciones.potenciometroV = 1.50f;
  mediciones.dhtValido = true;
  mediciones.potenciometroValido = true;
}

void loop() {
  unsigned long ahora = millis();

  // 1. Sondeo de Telegram (si hay Wi-Fi e Internet)
  Telegram::actualizar(mediciones, ahora);

  // 2. Procesamiento desde Monitor Serie (permite probar sin depender de Telegram)
  if (Serial.available() > 0) {
    String linea = Serial.readStringUntil('\n');
    linea.trim();
    if (linea.length() > 0) {
      Serial.println("\n[SERIE ENTRADA] -> " + linea);
      ResultadoComando r = Comandos::ejecutar(linea, OrigenComando::Serie, mediciones, ahora);
      Serial.println("[RESPUESTA]:\n" + r.respuesta);
    }
  }

  delay(10);
}