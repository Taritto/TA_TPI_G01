#pragma once
#include <Arduino.h>
#include <WiFi.h>

namespace Conectividad {
  inline void iniciar(const char* ssid, const char* clave) {
    WiFi.begin(ssid, clave);
    Serial.print("Conectando a WiFi: ");
    Serial.println(ssid);

    // Espera con timeout hasta conectar
    int intentos = 0;
    while (WiFi.status() != WL_CONNECTED && intentos < 30) {
      delay(500);
      Serial.print(".");
      intentos++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi conectado exitosamente!");
      Serial.print("Direccion IP obtenida: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\nNo se pudo conectar a WiFi (Timeout).");
    }
  }

  inline void actualizar(unsigned long ahoraMs) {}

  inline bool estaConectado() {
    return WiFi.status() == WL_CONNECTED;
  }
}