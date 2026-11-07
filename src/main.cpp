// Punto de entrada: coordina los módulos privados de lib/.
#include <Arduino.h>

#include "Actuadores.h"
#include "Comandos.h"
#include "Conectividad.h"
#include "Pantalla.h"
#include "Sensores.h"
#include "Telegram.h"

#if __has_include("Secretos.h")
#include "Secretos.h"
#else
#include "Secretos.example.h"
#endif

namespace {
void procesarComandoSerie(const Mediciones& mediciones, unsigned long ahoraMs) {
  if (Serial.available() <= 0) return;

  String texto = Serial.readStringUntil('\n');
  texto.trim();
  if (texto.length() == 0) return;

  const ResultadoComando resultado =
      Comandos::ejecutar(texto, OrigenComando::Serie, mediciones, ahoraMs);
  if (resultado.respuesta.length() > 0) {
    Serial.println(resultado.respuesta);
  } else {
    Serial.println("Comando no reconocido");
  }
}
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(20);

  Actuadores::iniciar();
  Sensores::iniciar();
  if (!Pantalla::iniciar()) {
    Serial.println("OLED no disponible; el sistema continua");
  }

  if (!Conectividad::iniciar()) {
    Serial.println("El sistema sigue funcionando en modo local");
  }
  Telegram::iniciar(BOT_TOKEN_LOCAL);
  Serial.println("Sistema ESP32 inicializado");
}

void loop() {
  const unsigned long ahoraMs = millis();
  Conectividad::actualizar(ahoraMs);
  Sensores::actualizar(ahoraMs);
  const Mediciones mediciones = Sensores::obtenerMediciones();

  procesarComandoSerie(mediciones, ahoraMs);
  Actuadores::actualizar(mediciones);
  Telegram::actualizar(mediciones, ahoraMs);
  Pantalla::actualizar(mediciones, Actuadores::obtenerEstado(), ahoraMs);
}
