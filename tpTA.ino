// ======================================================
// TRABAJO PRACTICO INTEGRADOR - TA 4K01
// Sistema embebido ESP32
// ======================================================

// ======================================================
// 1. INCLUSION DE LIBRERIAS
// ======================================================

#include "Sensores.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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
void mostrarEstadoLed();
void mostrarEstadoPot();
void mostrarEstadoDHT();
void procesarMensajesTelegram(int cantidadMensajes);
void consultarTelegram();

// ======================================================
// 2. CONFIGURACION DE PINES
// ======================================================

// LEDs
#define LED_VERDE 23
#define LED_AZUL 2

// Display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define OLED_SDA 21
#define OLED_SCL 22

// ======================================================
// 3. DECLARACION DE OBJETOS
// ======================================================

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ======================================================
// 4. VARIABLES GLOBALES
// ======================================================

// Adaptador temporal para los consumidores existentes. Solo Sensores lee
// hardware; estas copias se reemplazaran al integrar los demas modulos.
float temperatura = NAN;
float humedad = NAN;

int lecturaPotenciometro = 0;
float voltajePotenciometro = NAN;

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

// Historial de los ultimos 3 comandos de Telegram
String historialComandos[3] = {"Sin comandos", "Sin comandos", "Sin comandos"};

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

  Sensores::iniciar();
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Error al inicializar el display OLED");
    while (true) {
      delay(100);
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.println("Sistema ESP32");
  display.println("TA 4K01");
  display.println();
  display.println("Inicializando...");
  display.display();

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
  Sensores::actualizar(millis());
  const Mediciones mediciones = Sensores::obtenerMediciones();
  temperatura = mediciones.temperaturaC;
  humedad = mediciones.humedadPorcentaje;
  lecturaPotenciometro = mediciones.potenciometroADC;
  voltajePotenciometro = mediciones.potenciometroV;

  // Diagnostico cada segundo, sin frenar el muestreo del potenciometro.
  static unsigned long ultimoDiagnosticoMs = 0;
  const unsigned long ahoraMs = millis();
  if (ahoraMs - ultimoDiagnosticoMs >= 1000UL) {
    ultimoDiagnosticoMs = ahoraMs;
    if (mediciones.dhtValido) {
      Serial.print("Temperatura: ");
      Serial.print(temperatura);
      Serial.println(" °C");
      Serial.print("Humedad: ");
      Serial.print(humedad);
      Serial.println(" %");
    } else {
      Serial.println("DHT22: sin lectura valida");
    }
    if (mediciones.potenciometroValido) {
      Serial.print("Lectura ADC: ");
      Serial.println(lecturaPotenciometro);
      Serial.print("Voltaje potenciometro: ");
      Serial.print(voltajePotenciometro);
      Serial.println(" V");
    } else {
      Serial.println("Potenciometro: sin lectura valida");
    }
  }

  // PROCESAMIENTO DE COMANDOS
  procesarComandoSerie();

  // CONTROL AUTOMATICO
  controlarModoAutomatico();

  // CONSULTAR TELEGRAM
  consultarTelegram();

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
    bool debeEncender = voltajePotenciometro > limite;
    // El loop ya no espera un segundo: escribir y anunciar solo los cambios.
    if (debeEncender == estadoLedAzul) {
      return;
    }
    if (debeEncender) {
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
      mostrarEstadoLed();
    } else if (comando == "displaypot") {
      mostrarEstadoPot();
    } else if (comando == "displaydht") {
      mostrarEstadoDHT();
    } else {
      Serial.println("Comando no reconocido");
    }
  }
}

// ======================================================
// PANTALLA OLED
// ======================================================

void mostrarEstadoLed() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("ESTADO DE LOS LEDS");
  display.println("------------------");
  display.print("LED Verde: ");
  display.println(estadoLedVerde ? "ON" : "OFF");
  display.print("LED Azul: ");
  display.println(estadoLedAzul ? "ON" : "OFF");

  mostrarHistorialEnPantalla(32);

  display.display();
}

void mostrarEstadoPot() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("POTENCIOMETRO");
  display.println("------------------");
  display.print("ADC: ");
  display.println(lecturaPotenciometro);
  display.print("Voltaje: ");
  display.print(voltajePotenciometro);
  display.println(" V");

  mostrarHistorialEnPantalla(32);

  display.display();
}

void mostrarEstadoDHT() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("SENSOR DHT");
  display.println("------------------");
  if (isnan(temperatura) || isnan(humedad)) {
    display.println("Error de lectura");
  } else {
    display.print("Temperatura: ");
    display.print(temperatura);
    display.println(" C");
    display.print("Humedad: ");
    display.print(humedad);
    display.println(" %");
  }

  mostrarHistorialEnPantalla(32);

  display.display();
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
      enviarDatosThingSpeak();
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
      mostrarEstadoLed();
      bot.sendMessage(chat_id, "Estado de los LEDs mostrado en OLED.", "");
    }
    else if (texto == "/displaypot") {
      mostrarEstadoPot();
      bot.sendMessage(chat_id, "Estado del potenciometro mostrado en OLED.", "");
    }
    else if (texto == "/displaydht") {
      mostrarEstadoDHT();
      bot.sendMessage(chat_id, "Estado del DHT mostrado en OLED.", "");
    }
    // COMANDO DESCONOCIDO
    else {

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
      texto == "/ledazulon" ||
      texto == "/ledazuloff" ||
      texto == "/displayled" ||
      texto == "/displaypot" ||
      texto == "/displaydht" ||
      texto == "/thingup" ||
      (texto.startsWith("/umbral ") &&
      texto.substring(8).toFloat() >= 1.0 &&
      texto.substring(8).toFloat() <= 1.5)
    ) {

      guardarComando(texto);

      // Actualizamos la pantalla correspondiente
      if (texto == "/displayled") {
        mostrarEstadoLed();
      }
      else if (texto == "/displaypot") {
        mostrarEstadoPot();
      }
      else if (texto == "/displaydht") {
        mostrarEstadoDHT();
      }
      else {
        mostrarHistorialComandos();
      }

    }
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

// HISTORIAL DE LOS ULTIMOS 3 COMANDOS
void guardarComando(String comando) {

  // Desplazamos los comandos anteriores
  historialComandos[2] = historialComandos[1];
  historialComandos[1] = historialComandos[0];

  // Guardamos el nuevo comando
  historialComandos[0] = comando;
}

// MOSTRAR HISTORIAL EN OLED
void mostrarHistorialComandos() {
  display.clearDisplay();
  display.setCursor(0, 0);

  display.println("ULTIMOS COMANDOS");
  display.println("------------------");

  for (int i = 0; i < 3; i++) {
    display.print(i + 1);
    display.print(". ");
    display.println(historialComandos[i]);
  }

  display.display();
}


// MOSTRAR HISTORIAL EN UNA POSICION DEL OLED
void mostrarHistorialEnPantalla(int posicionY) {
  display.setCursor(0, posicionY);
  display.println("------------");

  for (int i = 0; i < 3; i++) {
    display.setCursor(0, posicionY + 8 + (i * 8));
    display.print(i + 1);
    display.print(". ");
    display.println(historialComandos[i]);
  }
}


// ======================================================
// ENVIO DE DATOS A THINGSPEAK
// ======================================================

void enviarDatosThingSpeak() {

  // Verificamos que el WiFi este conectado
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Error: WiFi desconectado.");
    return;
  }

  // Verificamos que las mediciones sean validas
  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("Error: datos del DHT11 invalidos.");
    return;
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
  if (codigoRespuesta > 0) {
    String respuesta = http.getString();
    Serial.print("Respuesta ThingSpeak: ");
    Serial.println(respuesta);
    if (respuesta.toInt() > 0) {
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
}


