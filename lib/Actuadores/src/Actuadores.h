#pragma once

#include "Tipos.h"

namespace Actuadores {
  void iniciar();
  bool fijarLedVerde(bool encendido);
  bool fijarLedAzulManual(bool encendido);
  bool activarAutomatico(float voltajeActualV);
  void desactivarAutomatico();
  bool configurarUmbral(float valorV);
  void actualizar(const Mediciones& mediciones);
  EstadoActuadores obtenerEstado();
}
