# Validación de Pantalla

Fecha: 2 de noviembre de 2026.

## Comprobaciones realizadas

- Compilación real con Arduino CLI, objetivo `esp32:esp32:esp32`, core ESP32
  2.0.17, Adafruit SSD1306 2.5.13, GFX 1.12.4 y BusIO 1.17.4: exitosa.
  Programa: 296901 bytes (22 %). Variables globales: 22024 bytes (6 %).
  Se compiló `libraries/Pantalla/examples/PruebaPantalla` enlazando la librería
  mediante `--libraries ./libraries`; no se compiló el sketch monolítico original.
- `git diff --cached --check`: sin errores de espacios.
- `diagram.json`: JSON válido, ESP32 y OLED con seis conexiones declaradas.
- `python wokwi/preparar.py`: genera los archivos del proyecto web.
- Comparación byte a byte: las cuatro fuentes generadas para Wokwi coinciden con
  las de `libraries/Pantalla/src`.

## Simulación pendiente

No se ejecutó la secuencia en Wokwi. Se abrió un proyecto ESP32, pero el control
del navegador falló antes de cargar las fuentes. No hay evidencia visual ni un
enlace guardado que certifique la simulación.

El ejemplo imprime los pasos de prueba, no aserciones sobre los píxeles. Revisar
visualmente las diez etapas y repetir con SDA desconectado según README_PANTALLA.md.
No se probó en hardware físico ni se integró con Telegram o el sketch completo.
