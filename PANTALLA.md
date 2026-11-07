# Biblioteca Pantalla

Se sigue la estructura de `libreria/sensores`: archivos del módulo junto al
sketch, tipos compartidos mínimos y adaptación del `.ino` a la interfaz pública.

| Archivo | Responsabilidad |
|---|---|
| `Pantalla.h` | Interfaz pública del módulo |
| `Pantalla.cpp` | Único objeto OLED, vista persistente e historial de tres comandos |
| `Tipos.h` | `Mediciones`, `EstadoActuadores` y `VistaPantalla` |
| `Config.h` | Pines I2C, dimensiones y dirección OLED |
| `tpTA.ino` | Inicialización, selección de vista y entrega de instantáneas |

`Tipos.h` y `Config.h` son compartidos. Al integrar con sensores, conservar una
sola definición de `Mediciones` y reunir sus constantes en el mismo `Config.h`.
Esta rama parte de `main`; no incorpora la implementación de Sensores.

## Interfaz

```cpp
bool Pantalla::iniciar();
void Pantalla::seleccionarVista(VistaPantalla vista);
void Pantalla::mostrarErrorComando(const String& comando);
void Pantalla::registrarComandoExitoso(const String& comando);
void Pantalla::actualizar(const Mediciones& mediciones,
                         const EstadoActuadores& estado,
                         unsigned long ahoraMs);
```

`iniciar()` devuelve false si falla I2C, no hay ACK de la OLED o falla su
inicialización. El programa continúa y las funciones del módulo no dibujan.
La desconexión posterior al arranque no se detecta de forma continua.

La vista permanece hasta otra selección de display. Registrar un comando no
cambia la vista. Los errores `/display...` permanecen hasta seleccionar una
vista válida y no entran al historial. `seleccionarVista(Error)` se ignora:
para establecer el mensaje se usa `mostrarErrorComando()`.

Se reservan cuatro filas superiores y tres inferiores para el historial, con
separador en y=37. No hay salto automático de línea: `~` indica truncado y `?`
representa caracteres fuera de la fuente ASCII. Los cambios de vista e historial
se dibujan en la siguiente actualización; las mediciones se refrescan cada
200 ms si el llamador actualiza con esa frecuencia. Pantalla no usa `delay()`.

## Integración

El sketch ya usa el módulo: se retiraron su objeto OLED, inicialización antigua,
historial y funciones de dibujo. `actualizarPantalla()` adapta las variables
existentes a las estructuras compartidas sin leer hardware adicional.
Se llama al final del lazo y después de cada mensaje Telegram.

El llamador decide qué comandos se ejecutaron con éxito. No se registran
órdenes desconocidas, cambios de umbral rechazados ni control manual azul en
automático. ThingSpeak devuelve un booleano para registrar `/thingup` solo si
su solicitud obtiene HTTP 2xx e identificador positivo según el criterio de
la base. Esto no sustituye el futuro módulo Telemetria ni su contrato completo.
Las órdenes por serie cambian la vista sin incorporarse al historial Telegram.

Las pausas y llamadas de red del sketch original aún limitan la frecuencia
efectiva de actualización; esta rama no implementa los demás módulos.

## Comprobación manual

En un sketch cuya carpeta se llame `tpTA`, copiar `tpTA.ino`, `Pantalla.h`,
`Pantalla.cpp`, `Tipos.h` y `Config.h`. Instalar las dependencias que incluye el
sketch. Para Wokwi usar ESP32 y SSD1306 128x64 en 0x3C: SDA 21, SCL 22, VCC 3V3
y GND común, además de los sensores y LEDs de la base.

Comprobar `/displayled`, `/displaypot` y `/displaydht`; luego `/estado` sin perder
la vista. Enviar cuatro comandos exitosos: solo deben quedar los tres últimos,
del más reciente al más antiguo. Probar `/displayxyz`, una lectura DHT inválida,
un comando largo y OLED desconectada antes de arrancar.

## Compilación verificada

El 3 de noviembre de 2026 se compiló el sketch completo con `Pantalla.cpp`
en una copia temporal cuya carpeta se llama `tpTA`, usando Arduino CLI y
`--fqbn esp32:esp32:esp32`. Resultado: 967601 bytes de programa (73 %) y
47928 bytes de variables globales (14 %).

Versiones usadas: Arduino-ESP32 2.0.17, DHT sensor library 1.4.7, Adafruit
Unified Sensor 1.1.15, SSD1306 2.5.13, GFX 1.12.4, BusIO 1.17.4,
UniversalTelegramBot 1.3.0 y ArduinoJson 6.21.6.

La simulación Wokwi y las pruebas físicas permanecen pendientes.
