#pragma once

#include "Tipos.h"

namespace Pantalla {
bool iniciar();
void seleccionarVista(VistaPantalla vista);
void mostrarErrorComando(const String& comando);
// El llamador verifica exito y origen Telegram antes de registrar.
void registrarComandoExitoso(const String& comando);
void actualizar(const Mediciones& mediciones,
                const EstadoActuadores& estado,
                unsigned long ahoraMs);
}
