// ======================================================
// TRABAJO PRACTICO INTEGRADOR - TA 4K01
// Sistema embebido ESP32
// ======================================================

// ======================================================
// 1. INCLUSION DE LIBRERIAS
// ======================================================

#include "Sensores.h"
#include "Pantalla.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <math.h>
#include "Actuadores.h"

// ======================================================
// PROTOTIPOS DE FUNCIONES (para evitar errores de scope)
// ======================================================
void conectarWiFi();
void procesarComandoSerie();
bool enviarDatosThingSpeak();
void procesarMensajesTelegram(int cantidadMensajes);
void consultarTelegram();

// ======================================================
// 2. CONFIGURACION DE PINES
// ======================================================

// ======================================================
// 4. VARIABLES GLOBALES
// ======================================================

float temperatura = 0.0;
float humedad = 0.0;

int lecturaPotenciometro = 0;
float voltajePotenciometro = 0.0;
Mediciones medicionesActuadores;

// Red WiFi
const char* WIFI_SSID = "Fibertel WiFi458 2.4GHz";
const char* WIFI_PASSWORD = "1142792048";

#define BOT_TOKEN "8825844790:AAEsbBG_NFcK8RbW7YoFKOskZ4UgYIXNVVQ"

WiFiClientSecure clienteTelegram;
UniversalTelegramBot bot(BOT_TOKEN, clienteTelegram);

unsigned long tiempoUltimaConsulta = 0;
const unsigned long INTERVALO_TELEGRAM = 1000;

// CONFIGURACION THINGSPEAK
const char* THINGSPEAK_API_KEY = "NAAK139LTSLRUOGD";
const char* THINGSPEAK_SERVER = "http://api.thingspeak.com/update";
// Control del intervalo de envio
unsigned long tiempoUltimoEnvioThingSpeak = 0;
const unsigned long INTERVALO_THINGSPEAK = 20000;

// ======================================================
// 5. SETUP()
// ======================================================

void setup() {
  Serial.begin(115200);

  Actuadores::iniciar();

  Sensores::iniciar();
  if (!Pantalla::iniciar()) {
    Serial.println("OLED no disponible; el sistema continua");
  }

  conectarWiFi();

  clienteTelegram.setInsecure();
  Serial.println("Bot de Telegram inicializado");

  Serial.println("================================");
  Serial.println(" SISTEMA ESP32 - TA 4K01");
  Serial.println(" Inicializacion completada");
  Serial.println("================================");

  delay(2000);
}

// ======================================================
// 6. LOOP()
// ======================================================

void loop() {
  // Las lecturas periódicas y no bloqueantes pertenecen al módulo Sensores.
  medicionesActuadores = Sensores::obtenerMediciones();
  Sensores::actualizar(millis());
  medicionesActuadores = Sensores::obtenerMediciones();
  temperatura = medicionesActuadores.temperaturaC;
  humedad = medicionesActuadores.humedadPorcentaje;
  lecturaPotenciometro = medicionesActuadores.potenciometroADC;
  voltajePotenciometro = medicionesActuadores.potenciometroV;

  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("Error al leer el sensor DHT");
  } else {
    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.println(" °C");

    Serial.print("Humedad: ");
    Serial.print(humedad);
    Serial.println(" %");
  }

  Serial.print("Lectura ADC: ");
  Serial.println(lecturaPotenciometro);
  Serial.print("Voltaje potenciometro: ");
  Serial.print(voltajePotenciometro);
  Serial.println(" V");

  // PROCESAMIENTO DE COMANDOS
  procesarComandoSerie();

  // CONTROL AUTOMATICO
  Actuadores::actualizar(medicionesActuadores);

  // CONSULTAR TELEGRAM
  consultarTelegram();
  Pantalla::actualizar(medicionesActuadores, Actuadores::obtenerEstado(), millis());

  Serial.println("--------------------------------");
  delay(1000);
}

// ======================================================
// CONTROL MANUAL DESDE MONITOR SERIE
// ======================================================

void procesarComandoSerie() {
  if (Serial.available() > 0) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    if (comando == "ledverdeon") {
      Actuadores::fijarLedVerde(true);
      Serial.println("LED VERDE: ENCENDIDO");
    } else if (comando == "ledverdeoff") {
      Actuadores::fijarLedVerde(false);
      Serial.println("LED VERDE: APAGADO");
    } else if (comando == "ledazulon") {
      Serial.println(Actuadores::fijarLedAzulManual(true) ? "LED AZUL: ENCENDIDO" : "LED azul bajo control automatico");
    } else if (comando == "ledazuloff") {
      Serial.println(Actuadores::fijarLedAzulManual(false) ? "LED AZUL: APAGADO" : "LED azul bajo control automatico");
    } else if (comando == "autoon") {
      Serial.println(Actuadores::activarAutomatico(voltajePotenciometro) ? "MODO AUTOMATICO ACTIVADO" : "No se pudo activar: voltaje invalido");
    } else if (comando == "autooff") {
      Actuadores::desactivarAutomatico();
      Serial.println("MODO AUTOMATICO DESACTIVADO");
    } else if (comando == "displayled") {
      Pantalla::seleccionarVista(VistaPantalla::Leds);
    } else if (comando == "displaypot") {
      Pantalla::seleccionarVista(VistaPantalla::Potenciometro);
    } else if (comando == "displaydht") {
      Pantalla::seleccionarVista(VistaPantalla::Dht);
    } else {
      if (comando.startsWith("display")) Pantalla::mostrarErrorComando(comando);
      Serial.println("Comando no reconocido");
    }
  }
}

// ======================================================
// CONEXION WIFI
// ======================================================

void conectarWiFi() {
  Serial.println();
  Serial.println("Conectando a WiFi...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado correctamente");
  Serial.print("Direccion IP: ");
  Serial.println(WiFi.localIP());
}

// ======================================================
// PROCESAMIENTO DE MENSAJES TELEGRAM
// ======================================================

void procesarMensajesTelegram(int cantidadMensajes) {
  bool envioExitoso = false;
  for (int i = 0; i < cantidadMensajes; i++) {
    String chat_id = bot.messages[i].chat_id;
    String texto = bot.messages[i].text;

    Serial.println("Mensaje recibido desde Telegram:");
    Serial.println(texto);

    if (texto == "/start") {
      String respuesta = "SISTEMA DE MONITOREO ESP32\n\n";
      respuesta += "COMANDOS GENERALES\n";
      respuesta += "/estado - Consultar estado general\n";
      respuesta += "/umbral <valor> - Configurar umbral\n";
      respuesta += "/autoon - Activar modo automatico\n";
      respuesta += "/autooff - Activar modo manual\n\n";
      respuesta += "CONTROL MANUAL DE LEDS\n";
      respuesta += "/ledverdeon - Encender LED verde\n";
      respuesta += "/ledverdeoff - Apagar LED verde\n";
      respuesta += "/ledazulon - Encender LED azul\n";
      respuesta += "/ledazuloff - Apagar LED azul\n\n";
      respuesta += "DISPLAY OLED\n";
      respuesta += "/displayled\n";
      respuesta += "/displaypot\n";
      respuesta += "/displaydht\n";
      bot.sendMessage(chat_id, respuesta, "");
    }
    else if (texto == "/ping") {
      bot.sendMessage(chat_id, "ESP32 funcionando correctamente!", "");
    }
    // ENVIO MANUAL A THINGSPEAK
    else if (texto == "/thingup") {
      envioExitoso = enviarDatosThingSpeak();
      bot.sendMessage(
        chat_id,
        "Se realizo el intento de envio de datos a ThingSpeak. Revise el Monitor Serie para conocer el resultado.",
        ""
      );

    }
    else if (texto == "/ledverdeon") {
      Actuadores::fijarLedVerde(true);
      bot.sendMessage(chat_id, "LED verde encendido.", "");
    }
    else if (texto == "/ledverdeoff") {
      Actuadores::fijarLedVerde(false);
      bot.sendMessage(chat_id, "LED verde apagado.", "");
    }
    else if (texto == "/ledazulon") {
      if (Actuadores::fijarLedAzulManual(true)) {
        bot.sendMessage(chat_id, "LED azul encendido.", "");
      } else {
        bot.sendMessage(chat_id, "Desactive el modo automatico con /autooff para controlar manualmente el LED azul.", "");
      }
    }
    else if (texto == "/ledazuloff") {
      if (Actuadores::fijarLedAzulManual(false)) {
        bot.sendMessage(chat_id, "LED azul apagado.", "");
      } else {
        bot.sendMessage(chat_id, "El LED azul esta siendo controlado por el modo automatico. Utilice /autooff para desactivarlo.", "");
      }
    }
    else if (texto == "/estado") {
      String respuesta = "ESTADO DEL SISTEMA\n\n";
      respuesta += "Temperatura: " + String(temperatura) + " C\n";
      respuesta += "Humedad: " + String(humedad) + " %\n";
      respuesta += "Potenciometro: " + String(voltajePotenciometro) + " V\n";
      const EstadoActuadores estadoActuadores = Actuadores::obtenerEstado();
      respuesta += "LED Verde: " + String(estadoActuadores.ledVerde ? "ENCENDIDO" : "APAGADO") + "\n";
      respuesta += "LED Azul: " + String(estadoActuadores.ledAzul ? "ENCENDIDO" : "APAGADO") + "\n";
      respuesta += "Modo: " + String(estadoActuadores.automatico ? "AUTOMATICO" : "MANUAL");
      bot.sendMessage(chat_id, respuesta, "");
    }
    else if (texto == "/autoon") {
      bot.sendMessage(chat_id, Actuadores::activarAutomatico(voltajePotenciometro) ? "Modo automatico activado correctamente." : "No se pudo activar el modo automatico: voltaje invalido.", "");
    }
    else if (texto == "/autooff") {
      Actuadores::desactivarAutomatico();
      bot.sendMessage(chat_id, "Modo manual activado correctamente.", "");
    }
    else if (texto.startsWith("/umbral ")) {
      float nuevoUmbral = texto.substring(8).toFloat();
      if (Actuadores::configurarUmbral(nuevoUmbral)) {
        bot.sendMessage(chat_id, "Umbral actualizado: " + String(nuevoUmbral, 2) + " V", "");
      } else {
        bot.sendMessage(chat_id, "Valor invalido. El umbral debe estar entre 1 y 1.5 V.", "");
      }
    }
    else if (texto == "/displayled") {
      Pantalla::seleccionarVista(VistaPantalla::Leds);
      bot.sendMessage(chat_id, "Estado de los LEDs mostrado en OLED.", "");
    }
    else if (texto == "/displaypot") {
      Pantalla::seleccionarVista(VistaPantalla::Potenciometro);
      bot.sendMessage(chat_id, "Estado del potenciometro mostrado en OLED.", "");
    }
    else if (texto == "/displaydht") {
      Pantalla::seleccionarVista(VistaPantalla::Dht);
      bot.sendMessage(chat_id, "Estado del DHT mostrado en OLED.", "");
    }
    // COMANDO DESCONOCIDO
    else {

      if (texto.startsWith("/display")) Pantalla::mostrarErrorComando(texto);
      bot.sendMessage(chat_id, "Comando no reconocido.", "");

    }

    // GUARDAR COMANDO VALIDO EN EL HISTORIAL
    if (
      texto == "/start" ||
      texto == "/ping" ||
      texto == "/estado" ||
      texto == "/autoon" ||
      texto == "/autooff" ||
      texto == "/ledverdeon" ||
      texto == "/ledverdeoff" ||
      (texto == "/ledazulon" && !Actuadores::obtenerEstado().automatico) ||
      (texto == "/ledazuloff" && !Actuadores::obtenerEstado().automatico) ||
      texto == "/displayled" ||
      texto == "/displaypot" ||
      texto == "/displaydht" ||
      (texto == "/thingup" && envioExitoso) ||
      (texto.startsWith("/umbral ") &&
      texto.substring(8).toFloat() >= 1.0 &&
      texto.substring(8).toFloat() <= 1.5)
    ) {

      Pantalla::registrarComandoExitoso(texto);
    }
    Pantalla::actualizar(medicionesActuadores, Actuadores::obtenerEstado(), millis());
  }
}

// CONSULTA DE MENSAJES TELEGRAM
void consultarTelegram() {
  if (millis() - tiempoUltimaConsulta >= INTERVALO_TELEGRAM) {
    int cantidadMensajes = bot.getUpdates(bot.last_message_received + 1);

    while (cantidadMensajes) {
      procesarMensajesTelegram(cantidadMensajes);
      cantidadMensajes = bot.getUpdates(bot.last_message_received + 1);
    }

    tiempoUltimaConsulta = millis();
  }
  /*
    // ENVIO AUTOMATICO A THINGSPEAK
  if (millis() - tiempoUltimoEnvioThingSpeak >= INTERVALO_THINGSPEAK) {

    enviarDatosThingSpeak();

  }
  */
}

// ======================================================
// ENVIO DE DATOS A THINGSPEAK
// ======================================================

bool enviarDatosThingSpeak() {

  // Verificamos que el WiFi este conectado
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Error: WiFi desconectado.");
    return false;
  }

  // Verificamos que las mediciones sean validas
  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("Error: datos del DHT11 invalidos.");
    return false;
  }

  // Creamos el cliente HTTP
  HTTPClient http;

  // Construimos la URL con los datos
  String url = String(THINGSPEAK_SERVER);
  url += "?api_key=";
  url += THINGSPEAK_API_KEY;
  url += "&field1=";
  url += String(temperatura, 2);
  url += "&field2=";
  url += String(humedad, 2);

  Serial.println("Enviando datos a ThingSpeak...");

  // Iniciamos la conexion HTTP
  http.begin(url);

  // Realizamos la solicitud GET
  int codigoRespuesta = http.GET();
  bool enviado = false;
  if (codigoRespuesta > 0) {
    String respuesta = http.getString();
    Serial.print("Respuesta ThingSpeak: ");
    Serial.println(respuesta);
    if (codigoRespuesta >= 200 && codigoRespuesta < 300 && respuesta.toInt() > 0) {
      enviado = true;
      Serial.println("Datos enviados correctamente.");
    } else {
      Serial.println("ThingSpeak no registro los datos.");
    }
  } else {
    Serial.print("Error HTTP: ");
    Serial.println(codigoRespuesta);
  }

  // Cerramos la conexion
  http.end();
  // Actualizamos el tiempo del ultimo intento
  tiempoUltimoEnvioThingSpeak = millis();
  return enviado;
}


