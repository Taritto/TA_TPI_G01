# Modulos privados

Cada carpeta contiene una libreria privada de PlatformIO bajo `src/`.
`src/main.cpp` coordina los modulos; `Configuracion` guarda los tipos y
valores compartidos. PlatformIO descubre las dependencias desde los `#include`.

- `Actuadores`, `Sensores` y `Pantalla`: hardware local.
- `WiFiManager` y `Conectividad`: conexión y estado de red.
- `ThingSpeakManager` y `Telemetria`: envío validado y limitado por intervalo.
- `Comandos` y `Telegram`: interpretación y transporte de comandos.
