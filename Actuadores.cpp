#include "Actuadores.h"

#include <Arduino.h>
#include <math.h>

#include "Config.h"

namespace Actuadores {
namespace {
  EstadoActuadores estado;

  bool voltajeValido(float voltajeV) {
    return isfinite(voltajeV) && voltajeV >= 0.0f && voltajeV <= 3.3f;
  }

  void escribirLedVerde(bool encendido) {
    estado.ledVerde = encendido;
    digitalWrite(LED_VERDE_PIN, encendido ? HIGH : LOW);
  }

  void escribirLedAzul(bool encendido) {
    estado.ledAzul = encendido;
    digitalWrite(LED_AZUL_PIN, encendido ? HIGH : LOW);
  }
}

void iniciar() {
  pinMode(LED_VERDE_PIN, OUTPUT);
  pinMode(LED_AZUL_PIN, OUTPUT);

  estado = EstadoActuadores{};
  estado.automatico = false;
  estado.lineaBaseV = 0.0f;
  estado.umbralV = 0.5f;
  escribirLedVerde(false);
  escribirLedAzul(false);
}

bool fijarLedVerde(bool encendido) {
  escribirLedVerde(encendido);
  return true;
}

bool fijarLedAzulManual(bool encendido) {
  if (estado.automatico) return false;
  escribirLedAzul(encendido);
  return true;
}

bool activarAutomatico(float voltajeActualV) {
  if (!voltajeValido(voltajeActualV)) return false;

  estado.lineaBaseV = voltajeActualV;
  estado.automatico = true;
  escribirLedAzul(voltajeActualV > estado.lineaBaseV + estado.umbralV);
  return true;
}

void desactivarAutomatico() {
  estado.automatico = false;
  escribirLedAzul(false);
}

bool configurarUmbral(float valorV) {
  if (!isfinite(valorV) || valorV < UMBRAL_MIN_V || valorV > UMBRAL_MAX_V) {
    estado.umbralV = UMBRAL_DEFECTO_V;
    return false;
  }

  estado.umbralV = valorV;
  return true;
}

void actualizar(const Mediciones& mediciones) {
  if (!estado.automatico) return;

  if (!mediciones.potenciometroValido || !voltajeValido(mediciones.potenciometroV)) {
    escribirLedAzul(false);
    return;
  }

  escribirLedAzul(mediciones.potenciometroV > estado.lineaBaseV + estado.umbralV);
}

EstadoActuadores obtenerEstado() {
  return estado;
}
}
