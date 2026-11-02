# Librería Pantalla

Rama: `libreria/pantalla`, basada en `main` (commit `8b671fd`).

Extrae la responsabilidad de OLED e historial del sketch original y conserva la
interfaz propuesta en la sección 6 de `PLAN_MODULOS.md`. Se distribuye como una
librería Arduino instalable en `libraries/Pantalla`, con `library.properties`,
`src/` y `examples/`. El ejemplo permite probar este módulo sin red ni credenciales.

## Uso

Instalar la carpeta `libraries/Pantalla` como librería Arduino y las dependencias
Adafruit SSD1306, Adafruit GFX Library y Adafruit BusIO. Abrir el ejemplo
`Pantalla > PruebaPantalla` y seleccionar ESP32 Dev Module.

```cpp
#include <Pantalla.h>

// En setup(), una sola vez:
// if (!Pantalla::iniciar()) Serial.println("OLED no disponible");

// Al ejecutar una orden de display válida:
// Pantalla::seleccionarVista(VistaPantalla::Potenciometro);

// Solo después de ejecutar con éxito un comando recibido por Telegram:
// Pantalla::registrarComandoExitoso(texto);

// Ante /display... desconocido (no se registra en el historial):
// Pantalla::mostrarErrorComando(texto);

// Al final de loop(), con las instantáneas actuales:
// Pantalla::actualizar(mediciones, estado, millis());
```

Pantalla no interpreta órdenes, no consulta sensores y no controla LEDs. El llamador
decide qué comandos tuvieron éxito. Las órdenes rechazadas y las de origen serie
no se registran. `seleccionarVista(Error)` no cambia la vista ni crea un mensaje.
El error persiste hasta una selección válida de display.

El área superior usa filas y=0, 8, 16 y 24; el separador está en y=37 y el historial
en y=40, 48 y 56. Se desactiva el salto de línea y se marca el truncado con `~`.
La fuente es ASCII; caracteres no representables se sustituyen por `?`.
Cambios de vista/historial se dibujan en la siguiente llamada a `actualizar()`;
las mediciones se refrescan cada 200 ms como máximo de espera, sin `delay()`.
El temporizador usa resta unsigned para tolerar el desbordamiento de `millis()`.

Si falla I2C, la detección por ACK o la reserva de memoria de la OLED, `iniciar()`
devuelve `false`; las otras funciones retornan sin dibujar. Esto comprueba ausencia
al arrancar, no garantiza detectar una desconexión posterior del display.

## Contratos e integración por el coordinador

`main` solo contenía `tpTA.ino`: todavía no tenía los contratos compartidos.
`src/Tipos.h` reproduce el bloque completo de tipos propuesto en el plan y
`src/Config.h` aporta únicamente las constantes OLED necesarias para esta prueba.
Estos archivos son provisión para compilar la librería por separado, no contratos
alternativos. Al integrar, acordar una única ubicación para ambos archivos con el
coordinador. No copiar otra definición de las estructuras al sketch: `#pragma once`
no evita redefiniciones entre dos archivos diferentes.

El sketch original se conserva para que el coordinador conecte el módulo en su
rama de integración. Antes de usar la librería en ese sketch debe:

1. Retirar su objeto `display`, el historial global y las funciones OLED antiguas.
2. Sustituir el bloque `Wire.begin`/inicialización OLED/bucle infinito de `setup()`
   por `Pantalla::iniciar()`, comprobando su resultado sin bloquear.
3. Sustituir `mostrarEstadoLed/Pot/DHT` por `seleccionarVista`.
4. Construir `Mediciones` y `EstadoActuadores` con las variables actuales y llamar
   `actualizar` al final de cada vuelta. Validar datos con bandera y finitud; cero
   es una lectura válida.
5. Sustituir `guardarComando` y los redibujos posteriores por
   `registrarComandoExitoso`, solo cuando la orden realmente se haya ejecutado.
   El filtro actual del sketch registra también órdenes manuales rechazadas;
   no reutilizar ese filtro sin corregirlo.
6. En `/display...` desconocido, avisar tanto a Pantalla como a Telegram.

## Compilar el ejemplo por línea de comandos

Desde la raíz del repositorio, con Arduino CLI instalado:

```powershell
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@2.0.17 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install "Adafruit SSD1306@2.5.13" "Adafruit GFX Library@1.12.4" "Adafruit BusIO@1.17.4"
arduino-cli compile --fqbn esp32:esp32:esp32 --libraries ./libraries --output-dir ./build ./libraries/Pantalla/examples/PruebaPantalla
```

## Probar en Wokwi web

1. Ejecutar `python wokwi/preparar.py`. Genera copias planas desde las fuentes
   originales, sin mantener una segunda implementación.
2. Crear un [proyecto ESP32](https://wokwi.com/projects/new/esp32).
3. En el menú junto a las pestañas del editor, elegir **Upload file(s)...**.
   Cargar los siete archivos de `wokwi/PruebaPantalla`: `sketch.ino`, `Pantalla.h`,
   `Pantalla.cpp`, `Tipos.h`, `Config.h`, `diagram.json` y `libraries.txt`.
   Reemplazar los archivos iniciales de mismo nombre si el editor lo solicita.
   No cargar `wokwi.toml` en la versión web.
4. Pulsar Play. El monitor serie a 115200 informa el caso actual cada 2,5 s.
   Las mediciones son ficticias e intencionales: esta prueba aísla Pantalla.
5. Revisar cada etapa con la siguiente tabla. Reiniciar para repetir la secuencia.

| Etapa | Comprobación visual esperada |
|---|---|
| Inicio | Sistema ESP32 y tres líneas `Sin comandos` |
| 1 | Verde ON, azul OFF, modo MANUAL |
| 2 | Potenciómetro ADC 2048 y 1.65 V |
| 3 | `/estado` entra al historial y permanece la vista Pot |
| 4 | DHT 24.5 C / 60.0 %; historial: `/displaydht`, `/estado`, `/displaypot` |
| 5 | DHT inválido: `Sin lectura` |
| 6 | NAN aun con bandera válida: `Sin lectura` |
| 7 | Error de display truncado con `~`; historial anterior intacto |
| 8 | `/ping` entra al historial y permanece el error |
| 9 | Selección Pot sale del error y muestra `Sin lectura` |
| 10 | Comando largo truncado; ninguna fila invade otra |

Para probar OLED ausente, detener, quitar la conexión SDA y reiniciar. Debe
imprimir `OLED ausente: continua el programa` y completar la secuencia por serie.
El caso de órdenes rechazadas se comprueba en la integración de Comandos: la
librería no tiene información suficiente para decidir si una orden fue exitosa.

Con Wokwi para VS Code o CLI se puede usar `wokwi.toml` después de compilar.
El CLI requiere una credencial de Wokwi propia; no incluirla en el repositorio.
La [documentación de Wokwi](https://docs.wokwi.com/guides/libraries) permite fijar
dependencias mediante `libraries.txt`; los archivos propios aquí van como fuentes
del sketch para evitar depender de la carga de librerías personalizadas de pago.

## Alcance de la validación

Ver `VALIDACION_PANTALLA.md` para el resultado real de las comprobaciones.
Compilar el ejemplo no certifica el funcionamiento del programa completo,
Telegram, sensores físicos ni la placa de la cátedra.
