#include "Tipos.h"
#include "Secretos.h"
#include "Conectividad.h"
#include "Actuadores.h"
#include "Pantalla.h"
#include "Telemetria.h"
#include "Comandos.h"
#include "Telegram.h"

const char* WIFI_SSID = WIFI_SSID_LOCAL;
const char* WIFI_PASS = WIFI_PASS_LOCAL;
const char* BOT_TOKEN = BOT_TOKEN_LOCAL;

Mediciones mediciones;

// Control anti-spam de alertas push
bool alertaVoltajeActiva = false;
unsigned long ultimoDisparoAlertaMs = 0;
const unsigned long COOLDOWN_ALERTA_MS = 15000; // 15 segundos entre alertas

void verificarAlertasVoltaje(const Mediciones& med, unsigned long ahoraMs) {
  EstadoActuadores estado = Actuadores::obtenerEstado();

  // Solo alertamos si el modo automático está activo y el potenciómetro es válido
  if (estado.automatico && med.potenciometroValido) {
    float limiteV = estado.lineaBaseV + estado.umbralV;

    // Disparo por flanco: Superó el límite y la alerta aún no estaba activa
    if (med.potenciometroV > limiteV && !alertaVoltajeActiva) {
      if (ahoraMs - ultimoDisparoAlertaMs >= COOLDOWN_ALERTA_MS) {
        String msg = "🚨 ALERTA DE VOLTAJE\n";
        msg += "⚡ Lectura actual: " + String(med.potenciometroV, 2) + " V\n";
        msg += "🎯 Límite: " + String(limiteV, 2) + " V (Base " + String(estado.lineaBaseV, 2) + " + Umbral " + String(estado.umbralV, 2) + ")\n";
        msg += "🔵 LED Azul encendido automáticamente.";

        if (Telegram::enviarAlerta(msg)) {
          alertaVoltajeActiva = true;
          ultimoDisparoAlertaMs = ahoraMs;
          Serial.println("[ALERTA PUSH] Enviada a Telegram con exito.");
        }
      }
    }
    // Retorno a la normalidad: El voltaje volvió a estar por debajo del límite
    else if (med.potenciometroV <= limiteV && alertaVoltajeActiva) {
      String msg = "✅ VOLTAJE NORMALIZADO\n";
      msg += "⚡ Lectura actual: " + String(med.potenciometroV, 2) + " V\n";
      msg += "🔵 LED Azul apagado.";

      if (Telegram::enviarAlerta(msg)) {
        alertaVoltajeActiva = false;
        ultimoDisparoAlertaMs = ahoraMs;
        Serial.println("[ALERTA PUSH] Normalizacion enviada a Telegram.");
      }
    }
  } else {
    // Si se desactiva el modo automático, rearmamos el estado
    alertaVoltajeActiva = false;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- SISTEMA ESP32 CON ALERTAS PUSH INICIADO ---");

  Actuadores::iniciar();
  Conectividad::iniciar(WIFI_SSID, WIFI_PASS);
  Telegram::iniciar(BOT_TOKEN);

  // Valores iniciales simulados
  mediciones.temperaturaC = 24.5f;
  mediciones.humedadPorcentaje = 60.0f;
  mediciones.potenciometroADC = 1860;
  mediciones.potenciometroV = 1.20f;
  mediciones.dhtValido = true;
  mediciones.potenciometroValido = true;
}

void loop() {
  unsigned long ahora = millis();

  // 1. Sondeo periódico de Telegram
  Telegram::actualizar(mediciones, ahora);

  // 2. Control y actualización de actuadores
  Actuadores::actualizar(mediciones);

  // 3. Comprobación y emisión de alertas push
  verificarAlertasVoltaje(mediciones, ahora);

  // 4. Procesamiento desde Monitor Serie (permite simular subidas de voltaje)
  if (Serial.available() > 0) {
    String linea = Serial.readStringUntil('\n');
    linea.trim();
    if (linea.length() > 0) {
      // Truco de prueba: si en la serie escribes "simpot 2.5", cambia el voltaje simulado
      if (linea.startsWith("simpot ")) {
        mediciones.potenciometroV = linea.substring(7).toFloat();
        Serial.println(">> Voltaje simulado cambiado a: " + String(mediciones.potenciometroV, 2) + " V");
      } else {
        ResultadoComando r = Comandos::ejecutar(linea, OrigenComando::Serie, mediciones, ahora);
        Serial.println("[RESPUESTA]:\n" + r.respuesta);
      }
    }
  }

  delay(10);
}