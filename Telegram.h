#pragma once
#include <Arduino.h>
#include "Tipos.h"

namespace Telegram {
  void iniciar(const char* token);
  void actualizar(const Mediciones& mediciones, unsigned long ahoraMs);
}