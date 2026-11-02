"""Genera archivos planos desde la libreria para Wokwi web (sin carga de libreria paga)."""
from pathlib import Path

repo = Path(__file__).resolve().parent.parent
destino = repo / "wokwi" / "PruebaPantalla"
lib = repo / "libraries" / "Pantalla"
for nombre in ("Pantalla.h", "Pantalla.cpp", "Tipos.h", "Config.h"):
    (destino / nombre).write_bytes((lib / "src" / nombre).read_bytes())
(destino / "sketch.ino").write_bytes(
    (lib / "examples" / "PruebaPantalla" / "PruebaPantalla.ino").read_bytes()
)
print(f"Archivos preparados en {destino}")
