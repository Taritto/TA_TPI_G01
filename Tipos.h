#pragma once

struct Mediciones {
  float potenciometroV = 0.0f;
  bool potenciometroValido = false;
};

struct EstadoActuadores {
  bool ledVerde = false;
  bool ledAzul = false;
  bool automatico = false;
  float lineaBaseV = 0.0f;
  float umbralV = 0.5f;
};
