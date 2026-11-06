#include "WiFiManager.h"

// Inicializamos los atributos con los datos pasados al constructor
WiFiManager::WiFiManager(const char* ssid, const char* password) {
  _ssid = ssid;
  _password = password;
}

// Lógica de conexión extraída de tu main.cpp
void WiFiManager::conectar() {
  Serial.println();
  Serial.println("Conectando a WiFi...");

  WiFi.begin(_ssid, _password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado correctamente");
  Serial.print("Direccion IP: ");
  Serial.println(WiFi.localIP());
}

// Verifica si el ESP32 sigue conectado a la red
bool WiFiManager::estaConectado() {
  return (WiFi.status() == WL_CONNECTED);
}