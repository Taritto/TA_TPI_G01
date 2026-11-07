#include "WiFiManager.h"

#include <string.h>

// Inicializamos los atributos con los datos pasados al constructor
WiFiManager::WiFiManager(const char* ssid, const char* password) {
  _ssid = ssid;
  _password = password;
}

// Lógica de conexión extraída de tu main.cpp
bool WiFiManager::conectar(unsigned long tiempoLimiteMs) {
  Serial.println();
  Serial.println("Conectando a WiFi...");

  WiFi.mode(WIFI_STA);
  if (strcmp(_ssid, "Wokwi-GUEST") == 0) {
    // El punto de acceso simulado de Wokwi está en el canal 6 y es abierto.
    WiFi.begin(_ssid, _password, 6);
  } else {
    WiFi.begin(_ssid, _password);
  }

  const unsigned long inicioMs = millis();
  unsigned long ultimoPuntoMs = inicioMs;
  while (WiFi.status() != WL_CONNECTED && millis() - inicioMs < tiempoLimiteMs) {
    delay(100);
    if (millis() - ultimoPuntoMs >= 500UL) {
      Serial.print(".");
      ultimoPuntoMs = millis();
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println();
    Serial.println("No se pudo conectar a WiFi; revise SSID y contraseña");
    WiFi.disconnect();
    return false;
  }

  Serial.println();
  Serial.println("WiFi conectado correctamente");
  Serial.print("Direccion IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

// Verifica si el ESP32 sigue conectado a la red
bool WiFiManager::estaConectado() {
  return (WiFi.status() == WL_CONNECTED);
}
