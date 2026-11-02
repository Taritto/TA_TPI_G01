# Biblioteca Sensores

Implementación del integrante 1, basada en la consigna 01 y en el contrato de `PLAN_MODULOS.md`.

## Archivos y responsabilidad

| Archivo | Contenido |
|---|---|
| `Sensores.h` | Las tres funciones públicas del módulo |
| `Sensores.cpp` | Único objeto DHT y únicas lecturas de DHT/ADC |
| `Tipos.h` | Tipo compartido `Mediciones`, mínimo necesario para esta integración |
| `Config.h` | Pines, tipo de sensor e intervalos de Sensores |
| `tpTA.ino` | Inicialización, actualización y adaptación a los consumidores existentes |
| `tests/sensores/` | Pruebas ejecutables con sustitutos de Arduino y DHT; no se cargan en la placa |

`Tipos.h` y `Config.h` son archivos compartidos del coordinador. Se incorporan aquí porque todavía no existían. Sus futuros cambios deben coordinarse con los demás integrantes. Los restantes tipos del plan se agregarán cuando sus módulos los necesiten.

## API

```cpp
Sensores::iniciar();
Sensores::actualizar(millis());
const Mediciones mediciones = Sensores::obtenerMediciones();
```

- `iniciar()` configura DHT22 en GPIO 33, potenciómetro en GPIO 32, resolución ADC de 12 bits y atenuación de 11 dB para ese pin. Descarta las mediciones anteriores y reinicia los temporizadores. Llamar desde `setup()`.
- `actualizar(unsigned long ahoraMs)` recibe una captura reciente de `millis()`, posterior a `iniciar()`. Lee el potenciómetro inmediatamente en la primera llamada y luego cada 100 ms como mínimo. Lee el DHT por primera vez después de 2000 ms desde la inicialización y luego cada 2000 ms como mínimo. No usa `delay()` ni espera a que venza un intervalo.
- `obtenerMediciones()` devuelve una copia del último estado. No lee hardware ni afecta los intervalos.

Si el lazo tarda más de un intervalo, la siguiente llamada hace una lectura, sin intentar recuperar todas las muestras omitidas. La lectura física del DHT utiliza las operaciones síncronas de su biblioteca; que no haya `delay()` en nuestro planificador no significa que cada lectura de hardware sea instantánea.

## Datos y errores

| Campo | Significado |
|---|---|
| `temperaturaC` | Temperatura en grados Celsius, o `NAN` si el par DHT es inválido |
| `humedadPorcentaje` | Humedad relativa en %, o `NAN` si el par DHT es inválido |
| `potenciometroADC` | Última lectura cruda de ADC; comprobar su indicador de validez |
| `potenciometroV` | Lectura calibrada de milivoltios dividida por 1000, o `NAN` si se rechaza |
| `dhtValido` | Ambas lecturas DHT son finitas y pertenecen al último intento |
| `potenciometroValido` | ADC entre 0 y 4095 y milivoltios entre 0 y 3300 |
| `ultimaLecturaDhtMs`, `ultimaLecturaPotMs` | Instante del último intento de muestreo de cada dispositivo |

Antes del primer muestreo, los indicadores están en `false`. Los valores flotantes están en `NAN`; ADC y marcas de tiempo empiezan en cero. Una marca cero también puede corresponder a una lectura real al inicio o tras desbordar el reloj: consultar siempre el indicador de validez.

Si una parte del par DHT falla, se invalidan ambas y se reemplazan por `NAN`; no se conserva una lectura antigua como si fuera reciente. El potenciómetro sigue actualizándose y viceversa. La siguiente lectura correcta recupera la validez automáticamente.

Un voltaje de cero es una medición válida. La API analógica no garantiza detectar un cable desconectado ni distinguir todos sus fallos de un cero real. Tampoco debe confundirse el rango lógico de 0 a 3.3 V del proyecto con una precisión garantizada en todo ese rango: comprobar calibración y saturación con la placa UTN.

El ADC crudo y los milivoltios se obtienen con dos conversiones consecutivas, conservando la estrategia de la base. Si el potenciómetro se mueve, no necesariamente corresponden al mismo instante.

## Integración realizada en el sketch

1. Se reemplazó el `#include <DHT.h>` por `#include "Sensores.h"`.
2. Se retiraron del `.ino` los pines de sensores y el objeto DHT. Se corrigió el tipo DHT11 a DHT22 en `Config.h`.
3. `setup()` llama a `Sensores::iniciar()`.
4. `loop()` actualiza el módulo y copia su instantánea a las variables que aún usan Telegram, OLED y ThingSpeak.
5. Se retiró el `delay(1000)` del lazo para permitir el muestreo del potenciómetro cada 100 ms. El diagnóstico serie conserva un intervalo de un segundo.
6. El control automático solo escribe/anuncia transiciones del azul, evitando repetir mensajes en cada vuelta del lazo más rápido.

Las variables antiguas de mediciones son un adaptador temporal, no otros lectores de hardware. Al integrar los demás módulos, sus consumidores deben recibir `Mediciones` y se eliminarán esas copias del `.ino`.

La conexión Wi-Fi inicial, las operaciones Telegram/HTTP y el lector serie de la base aún pueden demorar el lazo. Por eso 100/2000 ms son intervalos mínimos de muestreo, no plazos garantizados de tiempo real. Este cambio no implementa los otros módulos ni reactiva el envío periódico de ThingSpeak.

## Pruebas automáticas sin placa

Requisitos: Python 3 y un compilador `g++` con C++11. Desde la raíz del repositorio:

```powershell
python tests/sensores/ejecutar.py --cxx "C:\ruta\al\bin\g++.exe"
```

Si `g++` está en `PATH`, basta con `python tests/sensores/ejecutar.py`. El script compila el `Sensores.cpp` real junto a sustitutos de hardware y ejecuta las comprobaciones. El ejecutable se crea en una carpeta temporal y se elimina al finalizar. Los encabezados simulados solo se usan en esta compilación, no en el firmware.

Casos cubiertos: estado inicial, espera inicial DHT, primera lectura inmediata del potenciómetro, intervalos independientes, consulta sin muestreo, copias independientes, DHT con `NAN`/infinito, recuperación, voltaje cero e intermedio, extremo superior, valores fuera de rango, reinicialización y desbordamiento del reloj.

## Verificación pendiente en hardware

- Conectar DHT22 y potenciómetro en los GPIO definidos y compilar con DHT sensor library de Adafruit.
- Para Arduino IDE, abrir un sketch cuya carpeta tenga el mismo nombre que su `.ino`; copiar junto a él `Sensores.h`, `Sensores.cpp`, `Tipos.h` y `Config.h`. Mantener también las dependencias externas del sketch completo.
- Confirmar lecturas reales de temperatura/humedad y recorrer el potenciómetro de extremo a extremo, comparando voltaje con una referencia física.
- Desconectar el DHT: debe informarse lectura inválida mientras el potenciómetro sigue funcionando. Reconectar y verificar recuperación.
- Probar `/estado`, `/displaydht`, `/displaypot` y modo automático para verificar los consumidores existentes.

## Compilación del conjunto verificada

El 2 de noviembre de 2026 se compiló y enlazó el sketch completo, incluyendo `Sensores.cpp`, en una copia temporal para `esp32dev` con PlatformIO. No se cargó el firmware ni se cambió la estructura del repositorio a PlatformIO.

Entorno comprobado: plataforma Espressif32 7.1.3, framework Arduino-ESP32 2.0.17, DHT sensor library 1.4.7, Adafruit SSD1306 2.5.17, Adafruit GFX 1.13.0, Adafruit Unified Sensor 1.1.15, Adafruit BusIO 1.17.4, UniversalTelegramBot 1.3.0 y ArduinoJson 6.21.6. La compilación utilizó 48000 bytes de RAM y 988941 bytes de flash.

Para repetir esa compilación sin modificar el repositorio, crear un proyecto temporal, copiar el `.ino` y los cuatro archivos de Sensores/configuración/tipos a su carpeta `src` y usar este `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32@7.1.3
board = esp32dev
framework = arduino
lib_deps =
    adafruit/DHT sensor library@1.4.7
    adafruit/Adafruit SSD1306@2.5.17
    adafruit/Adafruit GFX Library@1.13.0
    adafruit/Adafruit Unified Sensor@1.1.15
    adafruit/Adafruit BusIO@1.17.4
    witnessmenow/UniversalTelegramBot@1.3.0
    bblanchon/ArduinoJson@6.21.6
```

Ejecutar `pio run --project-dir RUTA_DEL_PROYECTO_TEMPORAL`. La placa `esp32dev` es el objetivo de compilación usado para comprobar compatibilidad; validar la selección y el montaje físico de la placa UTN antes de cargar.

## Referencias técnicas

- [DHT sensor library de Adafruit](https://github.com/adafruit/DHT-sensor-library): comportamiento de lectura y mínimo de dos segundos entre muestras.
- [ADC de Arduino-ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html): lectura cruda, milivoltios calibrados, resolución y atenuación.
