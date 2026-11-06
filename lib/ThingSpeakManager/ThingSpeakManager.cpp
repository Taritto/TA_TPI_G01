#include "ThingSpeakManager.h"

// Inicialización de las variables de la clase
ThingSpeakManager::ThingSpeakManager(const char *apiKey, const char *server, unsigned long intervalo)
{
    _apiKey = apiKey;
    _server = server;
    _intervalo = intervalo;
    _tiempoUltimoEnvio = 0;
}

// Lógica de temporización con millis()
bool ThingSpeakManager::esTiempoDeEnviar()
{
    if (millis() - _tiempoUltimoEnvio >= _intervalo)
    {
        return true;
    }
    return false;
}

// Lógica de envío de datos extraída de tu main.cpp
void ThingSpeakManager::enviarDatos(float temperatura, float humedad)
{
    // Verificamos que el WiFi este conectado
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Error: WiFi desconectado.");
        return;
    }

    // Verificamos que las mediciones sean validas
    if (isnan(temperatura) || isnan(humedad))
    {
        Serial.println("Error: datos del DHT11 invalidos.");
        return;
    }

    // Creamos el cliente HTTP
    HTTPClient http;

    // Construimos la URL con los datos
    String url = String(_server);
    url += "?api_key=";
    url += _apiKey;
    url += "&field1=";
    url += String(temperatura, 2);
    url += "&field2=";
    url += String(humedad, 2);

    Serial.println("Enviando datos a ThingSpeak...");

    // Iniciamos la conexion HTTP
    http.begin(url);

    // Realizamos la solicitud GET
    int codigoRespuesta = http.GET();
    if (codigoRespuesta > 0)
    {
        String respuesta = http.getString();
        Serial.print("Respuesta ThingSpeak: ");
        Serial.println(respuesta);
        if (respuesta.toInt() > 0)
        {
            Serial.println("Datos enviados correctamente.");
        }
        else
        {
            Serial.println("ThingSpeak no registro los datos.");
        }
    }
    else
    {
        Serial.print("Error HTTP: ");
        Serial.println(codigoRespuesta);
    }

    // Cerramos la conexion
    http.end();

    // Actualizamos el tiempo del ultimo intento
    _tiempoUltimoEnvio = millis();
}