#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
  private:
    const char* _ssid;
    const char* _password;

  public:
    // Constructor que recibe el nombre de la red y la contraseña
    WiFiManager(const char* ssid, const char* password);

    // Método para iniciar la conexión (reemplaza a conectarWiFi)
    bool conectar(unsigned long tiempoLimiteMs = 12000UL);

    // Método auxiliar por si necesitas verificar el estado desde el main
    bool estaConectado();
};

#endif
