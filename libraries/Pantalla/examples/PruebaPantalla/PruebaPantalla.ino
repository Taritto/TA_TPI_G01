#include <Pantalla.h>

// Instantaneas ficticias: prueba OLED sin Wi-Fi, Telegram ni sensores.
Mediciones mediciones;
EstadoActuadores estado;
unsigned long ultimoPasoMs = 0;
unsigned int paso = 0;
bool finalizado = false;

void setup() {
  Serial.begin(115200);
  mediciones.temperaturaC = 24.5f;
  mediciones.humedadPorcentaje = 60.0f;
  mediciones.potenciometroADC = 2048;
  mediciones.potenciometroV = 1.65f;
  mediciones.dhtValido = true;
  mediciones.potenciometroValido = true;
  estado.ledVerde = true;
  Serial.println(Pantalla::iniciar() ? "OLED disponible" : "OLED ausente: continua el programa");
  ultimoPasoMs = millis();
}

void loop() {
  const unsigned long ahora = millis();
  if (!finalizado && ahora - ultimoPasoMs >= 2500) {
    ultimoPasoMs = ahora;
    switch (paso++) {
      case 0:
        Pantalla::seleccionarVista(VistaPantalla::Leds);
        Pantalla::registrarComandoExitoso("/displayled");
        Serial.println("1: LEDs verde ON, azul OFF");
        break;
      case 1:
        Pantalla::seleccionarVista(VistaPantalla::Potenciometro);
        Pantalla::registrarComandoExitoso("/displaypot");
        Serial.println("2: Pot 2048 / 1.65 V");
        break;
      case 2:
        Pantalla::registrarComandoExitoso("/estado");
        Serial.println("3: conserva vista Pot");
        break;
      case 3:
        Pantalla::seleccionarVista(VistaPantalla::Dht);
        Pantalla::registrarComandoExitoso("/displaydht");
        Serial.println("4: DHT 24.5 C / 60.0 %, historial dht, estado, pot");
        break;
      case 4:
        mediciones.dhtValido = false;
        Serial.println("5: DHT Sin lectura");
        break;
      case 5:
        mediciones.dhtValido = true;
        mediciones.temperaturaC = NAN;
        Serial.println("6: NAN con bandera valida tambien muestra Sin lectura");
        break;
      case 6:
        Pantalla::mostrarErrorComando("/displayxyz_comando_muy_largo");
        Serial.println("7: error truncado, conserva historial");
        break;
      case 7:
        Pantalla::registrarComandoExitoso("/ping");
        Serial.println("8: error persiste tras /ping");
        break;
      case 8:
        mediciones.potenciometroValido = false;
        Pantalla::seleccionarVista(VistaPantalla::Potenciometro);
        Serial.println("9: Pot Sin lectura; sale del error");
        break;
      case 9:
        Pantalla::registrarComandoExitoso("/comando_de_prueba_muy_largo");
        Serial.println("10: historial truncado con ~, sin superposicion");
        finalizado = true;
        Serial.println("SECUENCIA COMPLETA: comprobar OLED en cada paso");
        break;
    }
  }
  Pantalla::actualizar(mediciones, estado, ahora);
}
