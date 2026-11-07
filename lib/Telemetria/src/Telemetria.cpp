// Une las mediciones, el estado de red y el gestor ThingSpeak.
#include "Telemetria.h"

#include <math.h>

#include "Conectividad.h"
#include "ThingSpeakManager.h"

#if __has_include("Secretos.h")
#include "Secretos.h"
#else
#include "Secretos.example.h"
#endif

namespace {
constexpr unsigned long INTERVALO_MINIMO_MS = 20000UL;
unsigned long ultimoEnvioMs = 0;
bool huboEnvio = false;
ThingSpeakManager thingSpeak(THINGSPEAK_API_KEY_LOCAL);
}

namespace Telemetria {
ResultadoEnvio enviarAhora(const Mediciones& mediciones, unsigned long ahoraMs) {
  ResultadoEnvio resultado;
  if (!Conectividad::estaConectado()) {
    resultado.estado = EstadoEnvio::SinWifi;
    return resultado;
  }
  if (!mediciones.dhtValido || !isfinite(mediciones.temperaturaC) ||
      !isfinite(mediciones.humedadPorcentaje)) {
    resultado.estado = EstadoEnvio::DatosInvalidos;
    return resultado;
  }
  if (huboEnvio && ahoraMs - ultimoEnvioMs < INTERVALO_MINIMO_MS) {
    resultado.estado = EstadoEnvio::IntervaloNoCumplido;
    return resultado;
  }

  const long id = thingSpeak.enviarDatos(mediciones.temperaturaC,
                                         mediciones.humedadPorcentaje);
  ultimoEnvioMs = ahoraMs;
  huboEnvio = true;
  resultado.idRegistro = id;
  resultado.estado = id > 0 ? EstadoEnvio::Enviado : EstadoEnvio::RechazadoServidor;
  return resultado;
}
}
