# Sistema ESP32 — TA TPI G01

Firmware para leer temperatura, humedad y un potenciómetro; controlar dos LEDs; mostrar datos en una pantalla OLED; recibir comandos por serie o Telegram; y enviar mediciones a ThingSpeak.

## Carpetas

- `src/main.cpp`: inicia el hardware y coordina los módulos en cada ciclo.
- `lib/`: módulos privados del proyecto. `Sensores`, `Actuadores` y `Pantalla` manejan el hardware; `WiFiManager` y `Conectividad` la red; `ThingSpeakManager` y `Telemetria` los envíos; `Telegram` y `Comandos` los comandos.
- `lib/Configuracion/src/`: pines, tipos compartidos y plantilla de credenciales.
- `diagram.json` y `wokwi.toml`: circuito y configuración de simulación Wokwi.
- `platformio.ini`: placa, framework y dependencias necesarias.

## Preparar otra computadora

1. Instalar Visual Studio Code y la extensión **PlatformIO IDE**. Clonar el repositorio desde su URL y abrir la carpeta del proyecto.
2. PlatformIO descargará las dependencias declaradas en `platformio.ini` al compilar.
3. Para Wi-Fi real, copiar `lib/Configuracion/src/Secretos.example.h` como `lib/Configuracion/src/Secretos.h` y completar SSID, contraseña, token del bot y **Write API Key** de ThingSpeak. Para Wokwi, usar `Wokwi-GUEST` y contraseña vacía. No compartir ni subir `Secretos.h` al repositorio.

## Compilar y ejecutar

En la terminal de PlatformIO:

```sh
pio run                 # Compilar
pio run -t upload       # Subir a una placa ESP32 conectada
pio device monitor      # Abrir el monitor serie a 115200 baudios
```

Si PowerShell no reconoce `pio`, usar los botones **Build**, **Upload** y **Monitor** de PlatformIO en VS Code. En Windows también se puede invocar el ejecutable con `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run`.

Para Wokwi, compilar primero y luego ejecutar **F1 → Wokwi: Start Simulator**. La simulación usa `diagram.json`; la placa se conecta a `Wokwi-GUEST`.

En el monitor serie se pueden enviar `displayled`, `displaypot`, `displaydht`, `ledverdeon`, `ledverdeoff`, `ledazulon`, `ledazuloff`, `autoon` y `autooff`. En Telegram, enviar `/start` para abrir el menú. `/thingup` intenta subir las mediciones a ThingSpeak. Telegram y ThingSpeak requieren credenciales válidas y conexión a Internet.
