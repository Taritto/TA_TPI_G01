#include "Comandos.h"
#include "Actuadores.h"
#include "Pantalla.h"
#include "Telemetria.h"
#include <stdlib.h>

namespace Comandos {

ResultadoComando ejecutar(const String& texto,
                          OrigenComando origen,
                          const Mediciones& mediciones,
                          unsigned long ahoraMs) {
  ResultadoComando resultado;

  // 1. Normalización de entrada
  String cmd = texto;
  cmd.trim();

  // Si proviene de Serie sin '/', se le antepone para unificar el análisis
  if (cmd.length() > 0 && !cmd.startsWith("/")) {
    cmd = "/" + cmd;
  }

  // 2. Interpretación de comandos

  // COMANDO: /start (Ayuda estructurada)
  if (cmd == "/start") {
    resultado.reconocido = true;
    resultado.ejecutado = true;
    resultado.respuesta =
      "🤖 SISTEMA DE MONITOREO ESP32 - TA 4K01\n\n"
      "📋 COMANDOS GENERALES:\n"
      "/start - Muestra este menu de ayuda\n"
      "/ping - Verifica conectividad con la placa\n"
      "/estado - Consulta lecturas y estado actual\n"
      "/umbral <val> - Define umbral (1.0 a 1.5 V)\n"
      "/thingup - Envia mediciones a ThingSpeak\n\n"
      "⚙️ MODO AUTOMATICO:\n"
      "/autoon - Activa modo automatico (fija linea base)\n"
      "/autooff - Vuelve a modo manual (apaga LED azul)\n\n"
      "💡 CONTROL MANUAL LEDS:\n"
      "/ledverdeon - Enciende LED verde\n"
      "/ledverdeoff - Apaga LED verde\n"
      "/ledazulon - Enciende LED azul (solo en manual)\n"
      "/ledazuloff - Apaga LED azul (solo en manual)\n\n"
      "🖥️ PANTALLA OLED:\n"
      "/displayled - Muestra estado de LEDs\n"
      "/displaypot - Muestra lectura de potenciometro\n"
      "/displaydht - Muestra temperatura y humedad";
  }

  // COMANDO: /ping
  else if (cmd == "/ping") {
    resultado.reconocido = true;
    resultado.ejecutado = true;
    resultado.respuesta = "🏓 Pong! ESP32 funcionando correctamente.";
  }

  // COMANDO: /estado
  else if (cmd == "/estado") {
    EstadoActuadores estado = Actuadores::obtenerEstado();
    String r = "📊 ESTADO DEL SISTEMA\n\n";

    if (mediciones.dhtValido) {
      r += "🌡️ Temp: " + String(mediciones.temperaturaC, 1) + " °C\n";
      r += "💧 Humedad: " + String(mediciones.humedadPorcentaje, 1) + " %\n";
    } else {
      r += "🌡️ Temp: Sin lectura / Error\n";
      r += "💧 Humedad: Sin lectura / Error\n";
    }

    if (mediciones.potenciometroValido) {
      r += "⚡ Potenciometro: " + String(mediciones.potenciometroV, 2) + " V (ADC: " + String(mediciones.potenciometroADC) + ")\n";
    } else {
      r += "⚡ Potenciometro: Sin lectura / Error\n";
    }

    r += "🟢 LED Verde: " + String(estado.ledVerde ? "ENCENDIDO" : "APAGADO") + "\n";
    r += "🔵 LED Azul: " + String(estado.ledAzul ? "ENCENDIDO" : "APAGADO") + "\n";
    r += "⚙️ Modo: " + String(estado.automatico ? "AUTOMATICO" : "MANUAL") + "\n";
    r += "📏 Umbral: " + String(estado.umbralV, 2) + " V\n";
    if (estado.automatico) {
      r += "🎯 Linea Base: " + String(estado.lineaBaseV, 2) + " V\n";
    }

    resultado.reconocido = true;
    resultado.ejecutado = true;
    resultado.respuesta = r;
  }

  // CONTROL LED VERDE
  else if (cmd == "/ledverdeon") {
    resultado.reconocido = true;
    Actuadores::fijarLedVerde(true);
    Actuadores::actualizar(mediciones);
    resultado.ejecutado = true;
    resultado.respuesta = "LED verde encendido.";
  }
  else if (cmd == "/ledverdeoff") {
    resultado.reconocido = true;
    Actuadores::fijarLedVerde(false);
    Actuadores::actualizar(mediciones);
    resultado.ejecutado = true;
    resultado.respuesta = "LED verde apagado.";
  }

  // CONTROL LED AZUL (Condicionado a modo manual)
  else if (cmd == "/ledazulon") {
    resultado.reconocido = true;
    if (Actuadores::fijarLedAzulManual(true)) {
      Actuadores::actualizar(mediciones);
      resultado.ejecutado = true;
      resultado.respuesta = "LED azul encendido.";
    } else {
      resultado.ejecutado = false;
      resultado.respuesta = "Rechazado: El sistema esta en modo automatico. Use /autooff para desactivarlo.";
    }
  }
  else if (cmd == "/ledazuloff") {
    resultado.reconocido = true;
    if (Actuadores::fijarLedAzulManual(false)) {
      Actuadores::actualizar(mediciones);
      resultado.ejecutado = true;
      resultado.respuesta = "LED azul apagado.";
    } else {
      resultado.ejecutado = false;
      resultado.respuesta = "Rechazado: El sistema esta en modo automatico. Use /autooff para desactivarlo.";
    }
  }

  // MODO AUTOMATICO
  else if (cmd == "/autoon") {
    resultado.reconocido = true;
    if (!mediciones.potenciometroValido) {
      resultado.ejecutado = false;
      resultado.respuesta = "Error: Lectura de potenciometro no valida. No se puede activar modo automatico.";
    } else {
      if (Actuadores::activarAutomatico(mediciones.potenciometroV)) {
        Actuadores::actualizar(mediciones);
        EstadoActuadores estado = Actuadores::obtenerEstado();
        resultado.ejecutado = true;
        resultado.respuesta = "Modo automatico activado. Linea base: " + String(estado.lineaBaseV, 2) + " V.";
      } else {
        resultado.ejecutado = false;
        resultado.respuesta = "Error: Voltaje fuera de rango para linea base.";
      }
    }
  }
  else if (cmd == "/autooff") {
    resultado.reconocido = true;
    Actuadores::desactivarAutomatico();
    Actuadores::actualizar(mediciones);
    resultado.ejecutado = true;
    resultado.respuesta = "Modo automatico desactivado. Control manual restaurado.";
  }

  // CONFIGURACION DE UMBRAL: /umbral <valor>
  else if (cmd == "/umbral" || cmd.startsWith("/umbral ")) {
    resultado.reconocido = true;
    if (cmd == "/umbral") {
      Actuadores::configurarUmbral(NAN);
      Actuadores::actualizar(mediciones);
      resultado.ejecutado = false;
      resultado.respuesta = "Error: Falta parametro. Use /umbral <1.0 a 1.5>. Se restablecio umbral a 0.5 V.";
    } else {
      String arg = cmd.substring(8);
      arg.trim();
      char* endPtr = nullptr;
      float val = strtof(arg.c_str(), &endPtr);

      // Verificación estricta: que haya convertido algo, que no queden caracteres residuales y que sea finito
      if (arg.length() > 0 && endPtr != arg.c_str() && *endPtr == '\0' && !isnan(val) && !isinf(val)) {
        if (Actuadores::configurarUmbral(val)) {
          Actuadores::actualizar(mediciones);
          resultado.ejecutado = true;
          resultado.respuesta = "Umbral configurado exitosamente a " + String(val, 2) + " V.";
        } else {
          Actuadores::actualizar(mediciones);
          resultado.ejecutado = false;
          resultado.respuesta = "Valor fuera de rango [1.0, 1.5] V. Se restablecio el umbral a 0.5 V.";
        }
      } else {
        Actuadores::configurarUmbral(NAN);
        Actuadores::actualizar(mediciones);
        resultado.ejecutado = false;
        resultado.respuesta = "Formato invalido. Ingrese un numero decimal valido (ej: /umbral 1.2). Se restablecio a 0.5 V.";
      }
    }
  }

  // VISTAS DE PANTALLA OLED
  else if (cmd == "/displayled") {
    Pantalla::seleccionarVista(VistaPantalla::Leds);
    resultado.reconocido = true;
    resultado.ejecutado = true;
    resultado.respuesta = "Display OLED: Vista de LEDs seleccionada.";
  }
  else if (cmd == "/displaypot") {
    Pantalla::seleccionarVista(VistaPantalla::Potenciometro);
    resultado.reconocido = true;
    resultado.ejecutado = true;
    resultado.respuesta = "Display OLED: Vista de Potenciometro seleccionada.";
  }
  else if (cmd == "/displaydht") {
    Pantalla::seleccionarVista(VistaPantalla::Dht);
    resultado.reconocido = true;
    resultado.ejecutado = true;
    resultado.respuesta = "Display OLED: Vista de DHT seleccionada.";
  }
  else if (cmd.startsWith("/display")) {
    Pantalla::mostrarErrorComando(cmd);
    resultado.reconocido = false;
    resultado.ejecutado = false;
    resultado.respuesta = "Error: Comando de display no reconocido (" + cmd + ").";
  }

  // TELEMETRIA: /thingup
  else if (cmd == "/thingup") {
    resultado.reconocido = true;
    ResultadoEnvio envio = Telemetria::enviarAhora(mediciones, ahoraMs);
    switch (envio.estado) {
      case EstadoEnvio::Enviado:
        resultado.ejecutado = true;
        resultado.respuesta = "Datos enviados correctamente a ThingSpeak (Registro #" + String(envio.idRegistro) + ").";
        break;
      case EstadoEnvio::SinWifi:
        resultado.ejecutado = false;
        resultado.respuesta = "No se pudo enviar: Wi-Fi desconectado.";
        break;
      case EstadoEnvio::DatosInvalidos:
        resultado.ejecutado = false;
        resultado.respuesta = "No se pudo enviar: mediciones de sensor DHT invalidas.";
        break;
      case EstadoEnvio::IntervaloNoCumplido:
        resultado.ejecutado = false;
        resultado.respuesta = "Intento rechazado: debe respetar el intervalo minimo de 20s de ThingSpeak.";
        break;
      case EstadoEnvio::ErrorHttp:
        resultado.ejecutado = false;
        resultado.respuesta = "Error HTTP al conectar con ThingSpeak (Codigo: " + String(envio.codigoHttp) + ").";
        break;
      case EstadoEnvio::RechazadoServidor:
        resultado.ejecutado = false;
        resultado.respuesta = "ThingSpeak no registro los datos.";
        break;
      default:
        resultado.ejecutado = false;
        resultado.respuesta = "No se realizo el envio a ThingSpeak.";
        break;
    }
  }

  // COMANDO DESCONOCIDO
  else {
    resultado.reconocido = false;
    resultado.ejecutado = false;
    resultado.respuesta = "Comando no reconocido. Envie /start para ver las opciones disponibles.";
  }

  // 3. Registro en historial OLED (Solo si es orden exitosa proveniente de Telegram)
  if (origen == OrigenComando::Telegram && resultado.ejecutado) {
    Pantalla::registrarComandoExitoso(cmd);
  }

  return resultado;
}

} // namespace Comandos