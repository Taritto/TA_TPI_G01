#include "Tipos.h"
#include "Conectividad.h"
#include "Actuadores.h"
#include "Pantalla.h"
#include "Telemetria.h"
#include "Comandos.h"
#include "Telegram.h"

// Credenciales (En Wokwi usa "Wokwi-GUEST" y ""; en tu casa usa tu SSID y Password)
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

// Coloca aquí tu Bot Token real de Telegram
const char* BOT_TOKEN = "8825844790:AAEsbBG_NFcK8RbW7YoFKOskZ4UgYIXNVVQ";

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