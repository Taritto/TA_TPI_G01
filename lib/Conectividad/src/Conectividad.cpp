// Adaptador del gestor Wi-Fi aportado por la rama libreria/thingspeak-wifi.
#include "Conectividad.h"

#include "WiFiManager.h"

#if __has_include("Secretos.h")
#include "Secretos.h"
#else
#include "Secretos.example.h"
#endif

namespace {
WiFiManager wifi(WIFI_SSID_LOCAL, WIFI_PASS_LOCAL);
constexpr unsigned long INTERVALO_REINTENTO_MS = 30000UL;
unsigned long ultimoIntentoMs = 0;
}

namespace Conectividad {
bool iniciar() {
  ultimoIntentoMs = millis();
  return wifi.conectar();
}

void actualizar(unsigned long ahoraMs) {
  if (!wifi.estaConectado() && ahoraMs - ultimoIntentoMs >= INTERVALO_REINTENTO_MS) {
    ultimoIntentoMs = ahoraMs;
    wifi.conectar();
  }
}

bool estaConectado() { return wifi.estaConectado(); }
}
