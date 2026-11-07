#pragma once
#include <Arduino.h>
#include "Tipos.h"

namespace Comandos {
  ResultadoComando ejecutar(const String& texto,
                            OrigenComando origen,
                            const Mediciones& mediciones,
                            unsigned long ahoraMs);
}
