#ifndef THINGSPEAK_MANAGER_H
#define THINGSPEAK_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

class ThingSpeakManager
{
private:
    const char *_apiKey;
    const char *_server;
    unsigned long _tiempoUltimoEnvio;
    unsigned long _intervalo;

public:
    // Constructor para inicializar la configuración
    ThingSpeakManager(const char *apiKey, const char *server = "http://api.thingspeak.com/update", unsigned long intervalo = 20000);

    // Método para enviar la temperatura y humedad actuales
    long enviarDatos(float temperatura, float humedad);

    // Método opcional para comprobar si ya pasaron los 20 segundos
    bool esTiempoDeEnviar();
};

#endif
