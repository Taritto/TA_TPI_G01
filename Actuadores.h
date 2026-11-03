#pragma once
#include <Arduino.h>
#include "Tipos.h"

namespace Actuadores {
  inline EstadoActuadores& _estado() {
    static EstadoActuadores est;
    return est;
  }

  inline void iniciar() {
    _estado() = EstadoActuadores();
  }

  inline bool fijarLedVerde(bool encendido) {
    _estado().ledVerde = encendido;
    Serial.println(_estado().ledVerde ? "[STUB ACTUADOR] LED Verde: ON" : "[STUB ACTUADOR] LED Verde: OFF");
    return true;
  }

  inline bool fijarLedAzulManual(bool encendido) {
    if (_estado().automatico) return false;
    _estado().ledAzul = encendido;
    Serial.println(_estado().ledAzul ? "[STUB ACTUADOR] LED Azul: ON" : "[STUB ACTUADOR] LED Azul: OFF");
    return true;
  }

  inline bool activarAutomatico(float voltajeActualV) {
    if (isnan(voltajeActualV) || voltajeActualV < 0.0f || voltajeActualV > 3.3f) return false;
    _estado().automatico = true;
    _estado().lineaBaseV = voltajeActualV;
    _estado().ledAzul = false; // Inicialmente apagado hasta evaluar umbral
    Serial.println("[STUB ACTUADOR] Modo Automatico ACTIVADO con Linea Base: " + String(voltajeActualV) + " V");
    return true;
  }

  inline void desactivarAutomatico() {
    _estado().automatico = false;
    _estado().ledAzul = false;
    Serial.println("[STUB ACTUADOR] Modo Automatico DESACTIVADO");
  }

  inline bool configurarUmbral(float valorV) {
    if (!isnan(valorV) && valorV >= 1.0f && valorV <= 1.5f) {
      _estado().umbralV = valorV;
      Serial.println("[STUB ACTUADOR] Umbral configurado a: " + String(valorV) + " V");
      return true;
    }
    _estado().umbralV = 0.5f; // Restablece por defecto
    Serial.println("[STUB ACTUADOR] Umbral restablecido por defecto a: 0.5 V");
    return false;
  }

  inline void actualizar(const Mediciones& mediciones) {
    if (_estado().automatico) {
      if (mediciones.potenciometroValido) {
        _estado().ledAzul = (mediciones.potenciometroV > (_estado().lineaBaseV + _estado().umbralV));
      } else {
        // Política del proyecto: con lectura inválida apaga azul
        _estado().ledAzul = false;
      }
    }
  }

  inline EstadoActuadores obtenerEstado() {
    return _estado();
  }
}