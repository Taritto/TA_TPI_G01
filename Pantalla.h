#pragma once
#include <Arduino.h>
#include "Tipos.h"

namespace Pantalla {
  inline bool iniciar() { return true; }
  inline void seleccionarVista(VistaPantalla vista) {
    Serial.println("[STUB PANTALLA] Vista seleccionada: " + String((int)vista));
  }
  inline void mostrarErrorComando(const String& comando) {
    Serial.println("[STUB PANTALLA] ERROR mostrado para: " + comando);
  }
  inline void registrarComandoExitoso(const String& comando) {
    Serial.println("[STUB PANTALLA] Comando guardado en historial de 3: " + comando);
  }
  inline void actualizar(const Mediciones& mediciones, const EstadoActuadores& estado, unsigned long ahoraMs) {}
}