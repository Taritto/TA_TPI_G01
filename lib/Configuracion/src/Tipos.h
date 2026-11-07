#pragma once

#include <Arduino.h>
#include <math.h>

// Tipos compartidos por los módulos integrados.
struct Mediciones {
  float temperaturaC = NAN;
  float humedadPorcentaje = NAN;
  int potenciometroADC = 0;
  float potenciometroV = NAN;
  bool dhtValido = false;
  bool potenciometroValido = false;
  unsigned long ultimaLecturaDhtMs = 0;
  unsigned long ultimaLecturaPotMs = 0;
};

struct EstadoActuadores {
  bool ledVerde = false;
  bool ledAzul = false;
  bool automatico = false;
  float umbralV = 0.5f;
  float lineaBaseV = 0.0f;
};

enum class VistaPantalla { Inicio, Leds, Potenciometro, Dht, Error };
enum class OrigenComando { Telegram, Serie };
enum class EstadoEnvio {
  NoCorresponde,
  Enviado,
  SinWifi,
  DatosInvalidos,
  IntervaloNoCumplido,
  ErrorHttp,
  RechazadoServidor
};

struct ResultadoEnvio {
  EstadoEnvio estado = EstadoEnvio::NoCorresponde;
  int codigoHttp = 0;
  long idRegistro = 0;
};

struct ResultadoComando {
  bool reconocido = false;
  bool ejecutado = false;
  String respuesta;
};
