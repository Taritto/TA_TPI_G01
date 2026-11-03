#pragma once
#include <Arduino.h>
#include "Tipos.h"
#include "Conectividad.h"

namespace Telemetria {
  inline unsigned long& _ultimoEnvioMs() {
    static unsigned long ms = 0;
    return ms;
  }

  inline void iniciar(const char* apiKey, unsigned long ahoraMs) {}

  inline ResultadoEnvio enviarAhora(const Mediciones& mediciones, unsigned long ahoraMs) {
    ResultadoEnvio res;
    if (!Conectividad::estaConectado()) {
      res.estado = EstadoEnvio::SinWifi;
      return res;
    }
    if (!mediciones.dhtValido) {
      res.estado = EstadoEnvio::DatosInvalidos;
      return res;
    }
    if (_ultimoEnvioMs() != 0 && ahoraMs - _ultimoEnvioMs() < 20000) {
      res.estado = EstadoEnvio::IntervaloNoCumplido;
      return res;
    }
    _ultimoEnvioMs() = ahoraMs;
    res.estado = EstadoEnvio::Enviado;
    res.codigoHttp = 200;
    res.idRegistro = 12345;
    Serial.println("[STUB TELEMETRIA] Envio exitoso simulado a ThingSpeak");
    return res;
  }

  inline ResultadoEnvio actualizar(const Mediciones& mediciones, unsigned long ahoraMs) {
    ResultadoEnvio r;
    return r;
  }
}