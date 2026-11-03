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

// Mutex para compartir mediciones entre Core 0 y Core 1 de forma segura
SemaphoreHandle_t mutexMediciones = NULL;

// Control anti-spam de alertas push
bool alertaVoltajeActiva = false;
unsigned long ultimoDisparoAlertaMs = 0;
const unsigned long COOLDOWN_ALERTA_MS = 15000;

// ======================================================
// TAREA DE TELEGRAM (EJECUTA EN EL CORE 0)
// ======================================================
void tareaTelegramRed(void* parametro) {
  Serial.println("[FREERTOS] Tarea Telegram iniciada en Core: " + String(xPortGetCoreID()));

  for (;;) {
    if (Conectividad::estaConectado()) {
      Mediciones copiaMed;

      // Toma una copia segura de mediciones sin bloquear el Core 1
      if (xSemaphoreTake(mutexMediciones, pdMS_TO_TICKS(50)) == pdTRUE) {
        copiaMed = mediciones;
        xSemaphoreGive(mutexMediciones);
      }

      // Consulta y procesa mensajes de Telegram en segundo plano
      Telegram::actualizar(copiaMed, millis());
    }

    // Cede CPU para evitar disparar el Watchdog del sistema (100 ms)
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ======================================================
// EVALUACION DE ALERTAS PUSH
// ======================================================
void verificarAlertasVoltaje(const Mediciones& med, unsigned long ahoraMs) {
  EstadoActuadores estado = Actuadores::obtenerEstado();

  if (estado.automatico && med.potenciometroValido) {
    float limiteV = estado.lineaBaseV + estado.umbralV;

    // Disparo por flanco
    if (med.potenciometroV > limiteV && !alertaVoltajeActiva) {
      if (ahoraMs - ultimoDisparoAlertaMs >= COOLDOWN_ALERTA_MS) {
        String msg = "🚨 ALERTA DE VOLTAJE\n";
        msg += "⚡ Lectura actual: " + String(med.potenciometroV, 2) + " V\n";
        msg += "🎯 Límite: " + String(limiteV, 2) + " V (Base " + String(estado.lineaBaseV, 2) + " + Umbral " + String(estado.umbralV, 2) + ")\n";
        msg += "🔵 LED Azul encendido automáticamente.";

        if (Telegram::enviarAlerta(msg)) {
          alertaVoltajeActiva = true;
          ultimoDisparoAlertaMs = ahoraMs;
          Serial.println("\n[ALERTA PUSH] Enviada a Telegram con exito.");
        }
      }
    }
    // Retorno a la normalidad
    else if (med.potenciometroV <= limiteV && alertaVoltajeActiva) {
      String msg = "✅ VOLTAJE NORMALIZADO\n";
      msg += "⚡ Lectura actual: " + String(med.potenciometroV, 2) + " V\n";
      msg += "🔵 LED Azul apagado.";

      if (Telegram::enviarAlerta(msg)) {
        alertaVoltajeActiva = false;
        ultimoDisparoAlertaMs = ahoraMs;
        Serial.println("\n[ALERTA PUSH] Normalizacion enviada a Telegram.");
      }
    }
  } else {
    alertaVoltajeActiva = false;
  }
}

// ======================================================
// SETUP (CORE 1)
// ======================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- SISTEMA ESP32 MULTITAREA FREERTOS ---");

  // 1. Crear el mutex para sincronización entre núcleos
  mutexMediciones = xSemaphoreCreateMutex();

  // 2. Iniciar módulos
  Actuadores::iniciar();
  Conectividad::iniciar(WIFI_SSID, WIFI_PASS);
  Telegram::iniciar(BOT_TOKEN);

  // Valores iniciales
  mediciones.temperaturaC = 24.5f;
  mediciones.humedadPorcentaje = 60.0f;
  mediciones.potenciometroADC = 1860;
  mediciones.potenciometroV = 1.20f;
  mediciones.dhtValido = true;
  mediciones.potenciometroValido = true;

  // 3. Crear y anclar la tarea de Telegram al CORE 0
  // Parámetros: (Función, Nombre, StackBytes, Parametros, Prioridad, Handle, Núcleo)
  xTaskCreatePinnedToCore(
    tareaTelegramRed,
    "TareaTelegramRed",
    8192,
    NULL,
    1,
    NULL,
    0 // CORE 0
  );
}

// ======================================================
// LOOP PRINCIPAL (EJECUTA EN EL CORE 1 EN TIEMPO REAL)
// ======================================================
void loop() {
  unsigned long ahora = millis();

  // El Core 1 solo atiende hardware y control local (0 ms de espera de red)
  Actuadores::actualizar(mediciones);
  verificarAlertasVoltaje(mediciones, ahora);

  // Monitor Serie para pruebas locales
  if (Serial.available() > 0) {
    String linea = Serial.readStringUntil('\n');
    linea.trim();
    if (linea.length() > 0) {
      if (linea.startsWith("simpot ")) {
        if (xSemaphoreTake(mutexMediciones, pdMS_TO_TICKS(10)) == pdTRUE) {
          mediciones.potenciometroV = linea.substring(7).toFloat();
          xSemaphoreGive(mutexMediciones);
        }
        Serial.println("\n>> [SIMULACION] Voltaje cambiado a: " + String(mediciones.potenciometroV, 2) + " V");
      } else {
        ResultadoComando r = Comandos::ejecutar(linea, OrigenComando::Serie, mediciones, ahora);
        Serial.println("[RESPUESTA]:\n" + r.respuesta);
      }
    }
  }

  // Pequeño descanso de 10 ms para estabilidad de lazo
  delay(10);
}