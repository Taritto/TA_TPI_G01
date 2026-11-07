#pragma once

#include "Tipos.h"

namespace Sensores {

// Inicializa el hardware y descarta las mediciones anteriores.
void iniciar();

// Muestrea solo si vence el intervalo de cada dispositivo. No usa delay().
// Pasar una captura reciente de millis(), posterior a iniciar().
void actualizar(unsigned long ahoraMs);

// Devuelve una copia; no lee hardware ni modifica los temporizadores.
Mediciones obtenerMediciones();

}  // namespace Sensores
