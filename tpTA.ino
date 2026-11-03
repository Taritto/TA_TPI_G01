// ======================================================
// TRABAJO PRACTICO INTEGRADOR - TA 4K01
// Sistema embebido ESP32
// ======================================================

// ======================================================
// 1. INCLUSION DE LIBRERIAS
// ======================================================

#include <DHT.h>
#include "Pantalla.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// ======================================================
// PROTOTIPOS DE FUNCIONES (para evitar errores de scope)
// ======================================================
void conectarWiFi();
void encenderLedVerde();
void apagarLedVerde();
void encenderLedAzul();
void apagarLedAzul();
void activarModoAutomatico();
void desactivarModoAutomatico();
void controlarModoAutomatico();
void procesarComandoSerie();
void actualizarPantalla();
bool enviarDatosThingSpeak();
void procesarMensajesTelegram(int cantidadMensajes);
void consultarTelegram();

// ======================================================
// 2. CONFIGURACION DE PINES
// ======================================================

// Sensor DHT22 / DHT11
#define DHT_PIN 33
#define DHT_TYPE DHT11

// Potenciometro
#define POT_PIN 32

// LEDs
#define LED_VERDE 23
#define LED_AZUL 2

// ======================================================
// 3. DECLARACION DE OBJETOS
// ======================================================

DHT dht(DHT_PIN, DHT_TYPE);

// ======================================================
// 4. VARIABLES GLOBALES
// ======================================================

float temperatura = 0.0;
float humedad = 0.0;

int lecturaPotenciometro = 0;
float voltajePotenciometro = 0.0;

// Variables de control de actuadores
bool estadoLedVerde = false;
bool estadoLedAzul = false;

bool modoAutomatico = false;

float umbral = 0.5;
float lineaBase = 0.0;

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

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AZUL, OUTPUT);

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AZUL, LOW);

  dht.begin();
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
  // LECTURA DEL SENSOR DHT
  temperatura = dht.readTemperature();
  humedad = dht.readHumidity();

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

  // LECTURA DEL POTENCIOMETRO
  lecturaPotenciometro = analogRead(POT_PIN);
  int voltajeMilivoltios = analogReadMilliVolts(POT_PIN);
  voltajePotenciometro = voltajeMilivoltios / 1000.0;

  Serial.print("Lectura ADC: ");
  Serial.println(lecturaPotenciometro);
  Serial.print("Voltaje potenciometro: ");
  Serial.print(voltajePotenciometro);
  Serial.println(" V");

  // PROCESAMIENTO DE COMANDOS
  procesarComandoSerie();

  // CONTROL AUTOMATICO
  controlarModoAutomatico();

  // CONSULTAR TELEGRAM
  consultarTelegram();
  actualizarPantalla();

  Serial.println("--------------------------------");
  delay(1000);
}

// ======================================================
// FUNCIONES DE CONTROL DE LEDS
// ======================================================

void encenderLedVerde() {
  estadoLedVerde = true;
  digitalWrite(LED_VERDE, HIGH);
  Serial.println("LED VERDE: ENCENDIDO");
}

void apagarLedVerde() {
  estadoLedVerde = false;
  digitalWrite(LED_VERDE, LOW);
  Serial.println("LED VERDE: APAGADO");
}

void encenderLedAzul() {
  estadoLedAzul = true;
  digitalWrite(LED_AZUL, HIGH);
  Serial.println("LED AZUL: ENCENDIDO");
}

void apagarLedAzul() {
  estadoLedAzul = false;
  digitalWrite(LED_AZUL, LOW);
  Serial.println("LED AZUL: APAGADO");
}

// ======================================================
// FUNCIONES DE MODO AUTOMATICO
// ======================================================

void activarModoAutomatico() {
  modoAutomatico = true;
  lineaBase = voltajePotenciometro;

  Serial.println("MODO AUTOMATICO ACTIVADO");
  Serial.print("Linea base: ");
  Serial.print(lineaBase);
  Serial.println(" V");
}

void desactivarModoAutomatico() {
  modoAutomatico = false;
  apagarLedAzul();
  Serial.println("MODO AUTOMATICO DESACTIVADO");
}

void controlarModoAutomatico() {
  if (modoAutomatico) {
    float limite = lineaBase + umbral;
    if (voltajePotenciometro > limite) {
      encenderLedAzul();
    } else {
      apagarLedAzul();
    }
  }
}

// ======================================================
// CONTROL MANUAL DESDE MONITOR SERIE
// ======================================================

void procesarComandoSerie() {
  if (Serial.available() > 0) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    if (comando == "ledverdeon") {
      encenderLedVerde();
    } else if (comando == "ledverdeoff") {
      apagarLedVerde();
    } else if (comando == "ledazulon") {
      encenderLedAzul();
    } else if (comando == "ledazuloff") {
      apagarLedAzul();
    } else if (comando == "autoon") {
      activarModoAutomatico();
    } else if (comando == "autooff") {
      desactivarModoAutomatico();
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
// PANTALLA OLED
// ======================================================

// Adaptador temporal: el modulo recibe instantaneas, no accede a globales.
// Al integrar Sensores/Actuadores, pasar directamente sus resultados.
void actualizarPantalla() {
  Mediciones mediciones;
  mediciones.temperaturaC = temperatura;
  mediciones.humedadPorcentaje = humedad;
  mediciones.potenciometroADC = lecturaPotenciometro;
  mediciones.potenciometroV = voltajePotenciometro;
  mediciones.dhtValido = isfinite(temperatura) && isfinite(humedad);
  mediciones.potenciometroValido = isfinite(voltajePotenciometro) &&
      lecturaPotenciometro >= 0 && lecturaPotenciometro <= 4095 &&
      voltajePotenciometro >= 0.0f && voltajePotenciometro <= 3.3f;

  EstadoActuadores estado;
  estado.ledVerde = estadoLedVerde;
  estado.ledAzul = estadoLedAzul;
  estado.automatico = modoAutomatico;
  estado.umbralV = umbral;
  estado.lineaBaseV = lineaBase;
  Pantalla::actualizar(mediciones, estado, millis());
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
  for (int i = 0; i < cantidadMensajes; i++) {
    String chat_id = bot.messages[i].chat_id;
    String texto = bot.messages[i].text;
    bool envioExitoso = false;

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
      encenderLedVerde();
      bot.sendMessage(chat_id, "LED verde encendido.", "");
    }
    else if (texto == "/ledverdeoff") {
      apagarLedVerde();
      bot.sendMessage(chat_id, "LED verde apagado.", "");
    }
    else if (texto == "/ledazulon") {
      if (!modoAutomatico) {
        encenderLedAzul();
        bot.sendMessage(chat_id, "LED azul encendido.", "");
      } else {
        bot.sendMessage(chat_id, "Desactive el modo automatico con /autooff para controlar manualmente el LED azul.", "");
      }
    }
    else if (texto == "/ledazuloff") {
      if (!modoAutomatico) {
        apagarLedAzul();
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
      respuesta += "LED Verde: " + String(estadoLedVerde ? "ENCENDIDO" : "APAGADO") + "\n";
      respuesta += "LED Azul: " + String(estadoLedAzul ? "ENCENDIDO" : "APAGADO") + "\n";
      respuesta += "Modo: " + String(modoAutomatico ? "AUTOMATICO" : "MANUAL");
      bot.sendMessage(chat_id, respuesta, "");
    }
    else if (texto == "/autoon") {
      activarModoAutomatico();
      bot.sendMessage(chat_id, "Modo automatico activado correctamente.", "");
    }
    else if (texto == "/autooff") {
      desactivarModoAutomatico();
      bot.sendMessage(chat_id, "Modo manual activado correctamente.", "");
    }
    else if (texto.startsWith("/umbral ")) {
      float nuevoUmbral = texto.substring(8).toFloat();
      if (nuevoUmbral >= 1.0 && nuevoUmbral <= 1.5) {
        umbral = nuevoUmbral;
        bot.sendMessage(chat_id, "Umbral actualizado: " + String(umbral, 2) + " V", "");
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
      (texto == "/ledazulon" && !modoAutomatico) ||
      (texto == "/ledazuloff" && !modoAutomatico) ||
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
    actualizarPantalla();
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


