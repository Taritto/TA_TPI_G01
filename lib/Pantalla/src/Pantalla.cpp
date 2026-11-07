#include "Pantalla.h"
#include "Config.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace {
Adafruit_SSD1306 display(OLED_ANCHO, OLED_ALTO, &Wire, -1);
VistaPantalla vistaActual = VistaPantalla::Inicio;
String historial[3];
String comandoError;
bool disponible = false;
bool pendiente = true;
unsigned long ultimoRefrescoMs = 0;
constexpr unsigned long INTERVALO_REFRESCO_MS = 200;
constexpr unsigned int COLUMNAS = OLED_ANCHO / 6;

String acortar(const String& texto, unsigned int limite) {
  String limpio;
  // La fuente integrada es ASCII: no permitir controles ni partir UTF-8.
  for (unsigned int i = 0; i < texto.length() && limpio.length() <= limite; ++i) {
    const unsigned char c = texto[i];
    if (c >= 32 && c <= 126) limpio += char(c);
    else if (c < 128 || (c & 0xC0) != 0x80) limpio += '?';
  }
  if (limpio.length() > limite) return limpio.substring(0, limite - 1) + "~";
  return limpio;
}

void linea(int y, const String& texto) {
  display.setCursor(0, y);
  display.print(acortar(texto, COLUMNAS));
}

void dibujar(const Mediciones& m, const EstadoActuadores& e) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  switch (vistaActual) {
    case VistaPantalla::Inicio:
      linea(0, "Sistema ESP32");
      linea(8, "TA 4K01");
      linea(24, "Seleccione una vista");
      break;
    case VistaPantalla::Leds:
      linea(0, "ESTADO DE LOS LEDS");
      linea(8, String("Verde: ") + (e.ledVerde ? "ON" : "OFF"));
      linea(16, String("Azul: ") + (e.ledAzul ? "ON" : "OFF"));
      linea(24, e.automatico ? "Modo: AUTOMATICO" : "Modo: MANUAL");
      break;
    case VistaPantalla::Potenciometro:
      linea(0, "POTENCIOMETRO");
      if (m.potenciometroValido && isfinite(m.potenciometroV)) {
        linea(8, "ADC: " + String(m.potenciometroADC));
        linea(16, "Voltaje: " + String(m.potenciometroV, 2) + " V");
      } else linea(8, "Sin lectura");
      break;
    case VistaPantalla::Dht:
      linea(0, "SENSOR DHT");
      if (m.dhtValido && isfinite(m.temperaturaC) && isfinite(m.humedadPorcentaje)) {
        linea(8, "Temp: " + String(m.temperaturaC, 1) + " C");
        linea(16, "Humedad: " + String(m.humedadPorcentaje, 1) + " %");
      } else linea(8, "Sin lectura");
      break;
    case VistaPantalla::Error:
      linea(0, "ERROR DE COMANDO");
      linea(8, "Display desconocido");
      linea(24, comandoError);
      break;
  }
  display.drawFastHLine(0, 37, OLED_ANCHO, SSD1306_WHITE);
  for (unsigned int i = 0; i < 3; ++i) {
    linea(40 + i * 8, String(i + 1) + ". " + historial[i]);
  }
  display.display();
}
}

namespace Pantalla {
bool iniciar() {
  disponible = false;
  vistaActual = VistaPantalla::Inicio;
  comandoError = "";
  for (String& comando : historial) comando = "Sin comandos";
  pendiente = true;
  if (!Wire.begin(OLED_SDA, OLED_SCL)) return false;
  // SSD1306::begin comprueba memoria, pero no garantiza ACK del dispositivo.
  Wire.beginTransmission(OLED_DIRECCION);
  if (Wire.endTransmission() != 0) return false;
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_DIRECCION, true, false)) return false;
  disponible = true;
  dibujar(Mediciones{}, EstadoActuadores{});
  ultimoRefrescoMs = millis();
  pendiente = false;
  return true;
}

void seleccionarVista(VistaPantalla vista) {
  if (!disponible || vista == VistaPantalla::Error) return;
  if (vista != VistaPantalla::Inicio && vista != VistaPantalla::Leds &&
      vista != VistaPantalla::Potenciometro && vista != VistaPantalla::Dht) return;
  vistaActual = vista;
  pendiente = true;
}

void mostrarErrorComando(const String& comando) {
  if (!disponible) return;
  comandoError = acortar(comando, COLUMNAS);
  vistaActual = VistaPantalla::Error;
  pendiente = true;
}

void registrarComandoExitoso(const String& comando) {
  if (!disponible) return;
  historial[2] = historial[1];
  historial[1] = historial[0];
  historial[0] = acortar(comando, COLUMNAS - 3);
  pendiente = true;
}

void actualizar(const Mediciones& mediciones, const EstadoActuadores& estado,
                unsigned long ahoraMs) {
  if (!disponible) return;
  if (!pendiente && ahoraMs - ultimoRefrescoMs < INTERVALO_REFRESCO_MS) return;
  dibujar(mediciones, estado);
  ultimoRefrescoMs = ahoraMs;
  pendiente = false;
}
}
