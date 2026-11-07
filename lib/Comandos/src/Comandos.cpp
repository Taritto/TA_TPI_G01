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

  String cmd = texto;
  cmd.trim();

  // Solo si proviene del Monitor Serie sin '/', se le antepone para unificar el análisis
  if (origen == OrigenComando::Serie && cmd.length() > 0 && !cmd.startsWith("/")) {
    cmd = "/" + cmd;
  }

  // --- NAVEGACION DE SUBMENUS ---
  if (cmd == "/start" || cmd == "🔙 Menú Principal" || cmd == "/menu") {
    resultado.reconocido = true;
    resultado.ejecutado = true;
    resultado.respuesta =
      "🤖 ESP32 | Comandos disponibles\n"
      "\n"
      "COMANDO GENEREALES\n"
      "📊 /estado — Ver sensores y estado de LEDs.\n"
      "🏓 /ping — Comprobar que la placa responde.\n"
      "☁️ /thingup — Enviar mediciones a ThingSpeak.\n"
      "📏 /umbral <1.0 a 1.5> — Ajustar umbral automático.\n"
      "\n"
      "CONTROL DE LEDS\n"
      "🟢 /ledverdeon — Encender LED verde.\n"
      "⚫ /ledverdeoff — Apagar LED verde.\n"
      "🔵 /ledazulon — Encender LED azul en modo manual.\n"
      "⚫ /ledazuloff — Apagar LED azul en modo manual.\n"
      "🤖 /autoon — Activar control automático del LED azul.\n"
      "🖐️ /autooff — Volver al control manual.\n"
      "\n"
      "DISPLAY OLED\n"
      "🖥️ /displayled — Ver LEDs en la pantalla OLED.\n"
      "🎚️ /displaypot — Ver potenciómetro en la pantalla OLED.\n"
      "🌡️ /displaydht — Ver temperatura y humedad en la OLED.\n"
      "\n"
      "📋 /monitoreo — Abrir opciones de monitoreo.\n"
      "💡 /leds — Abrir controles de LEDs.\n"
      "⚙️ /auto — Abrir configuración automática.\n"
      "🖥️ /oled — Abrir opciones de pantalla OLED.\n"
      "📖 /ayuda — Ver guía detallada.\n"
      "🏠 /menu — Volver al menú principal.";
  }
  else if (cmd == "📋 Monitoreo General" || cmd == "/monitoreo") {
    resultado.reconocido = true;
    resultado.ejecutado = true;
    resultado.respuesta = "📋 MONITOREO GENERAL\nConsulte estado o envíe datos a ThingSpeak.\n(Para umbral escriba: /umbral <1.0 a 1.5>)";
  }
  else if (cmd == "💡 Control de LEDs" || cmd == "/leds") {
    resultado.reconocido = true;
    resultado.ejecutado = true;
    resultado.respuesta = "💡 CONTROL DE LEDS\nSeleccione el estado deseado:";
  }
  else if (cmd == "⚙️ Modo Automático" || cmd == "/auto") {
    resultado.reconocido = true;
    resultado.ejecutado = true;
    resultado.respuesta = "⚙️ MODO AUTOMÁTICO\nControl del LED azul por potenciómetro y umbral:";
  }
  else if (cmd == "🖥️ Pantalla OLED" || cmd == "/oled") {
    resultado.reconocido = true;
    resultado.ejecutado = true;
    resultado.respuesta = "🖥️ PANTALLA OLED\nSeleccione la vista para el display:";
  }
  else if (cmd == "(LED Azul en Modo Automático)") {
    resultado.reconocido = true;
    resultado.ejecutado = false;
    resultado.respuesta = "El LED azul se controla automáticamente por potenciómetro. Para control manual, desactive el modo automático en ⚙️ Modo Automático.";
  }
    else if (cmd == "📖 Guía de Comandos" || cmd == "/ayuda" || cmd == "/help") {
    resultado.reconocido = true;
    resultado.ejecutado = true;
    resultado.respuesta =
      "📖 Guía de comandos\n"
      "📊 /estado — Temperatura, humedad, voltaje y LEDs.\n"
      "🏓 /ping — Comprueba conectividad con la placa.\n"
      "☁️ /thingup — Publica las mediciones en ThingSpeak.\n"
      "📏 /umbral <1.0 a 1.5> — Configura el umbral automático en voltios.\n"
      "🟢 /ledverdeon — Enciende el LED verde.\n"
      "⚫ /ledverdeoff — Apaga el LED verde.\n"
      "🔵 /ledazulon — Enciende el LED azul si el modo es manual.\n"
      "⚫ /ledazuloff — Apaga el LED azul si el modo es manual.\n"
      "🤖 /autoon — Activa el control automático y toma el voltaje actual como base.\n"
      "🖐️ /autooff — Desactiva el modo automático y vuelve al control manual.\n"
      "🖥️ /displayled — Muestra el estado de LEDs en la OLED.\n"
      "🎚️ /displaypot — Muestra lectura ADC y voltaje en la OLED.\n"
      "🌡️ /displaydht — Muestra temperatura y humedad en la OLED.\n"
      "📋 /monitoreo — Abre el submenú de monitoreo.\n"
      "💡 /leds — Abre el submenú de LEDs.\n"
      "⚙️ /auto — Abre el submenú automático.\n"
      "🖥️ /oled — Abre el submenú de la pantalla.\n"
      "📖 /ayuda — Muestra esta guía.\n"
      "🏠 /menu — Vuelve al menú principal.";
  }

  // --- COMANDOS GENERALES ---
  else if (cmd == "/ping") {
    resultado.reconocido = true;
    resultado.ejecutado = true;
    resultado.respuesta = "🏓 Pong! ESP32 funcionando correctamente.";
  }
  else if (cmd == "/estado") {
    EstadoActuadores estado = Actuadores::obtenerEstado();
    String r = "📊 ESTADO ACTUAL\n";

    if (mediciones.dhtValido) {
      r += "🌡️ Temp: " + String(mediciones.temperaturaC, 1) + " °C | 💧 Hum: " + String(mediciones.humedadPorcentaje, 1) + " %\n";
    } else {
      r += "🌡️ Temp / Hum: Error o sin lectura\n";
    }

    if (mediciones.potenciometroValido) {
      r += "⚡ Pot: " + String(mediciones.potenciometroV, 2) + " V (ADC: " + String(mediciones.potenciometroADC) + ")\n";
    } else {
      r += "⚡ Pot: Sin lectura\n";
    }

    r += "🟢 Verde: " + String(estado.ledVerde ? "ON" : "OFF") + " | 🔵 Azul: " + String(estado.ledAzul ? "ON" : "OFF") + "\n";
    r += "⚙️ Modo: " + String(estado.automatico ? "AUTOMATICO" : "MANUAL") + " | 📏 Umbral: " + String(estado.umbralV, 2) + " V";
    if (estado.automatico) {
      r += "\n🎯 Línea Base: " + String(estado.lineaBaseV, 2) + " V";
    }

    resultado.reconocido = true;
    resultado.ejecutado = true;
    resultado.respuesta = r;
  }

  // --- CONTROL LEDS ---
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
  else if (cmd == "/ledazulon") {
    resultado.reconocido = true;
    if (Actuadores::fijarLedAzulManual(true)) {
      Actuadores::actualizar(mediciones);
      resultado.ejecutado = true;
      resultado.respuesta = "LED azul encendido.";
    } else {
      resultado.ejecutado = false;
      resultado.respuesta = "Rechazado: Sistema en modo automático. Use /autooff primero.";
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
      resultado.respuesta = "Rechazado: Sistema en modo automático. Use /autooff primero.";
    }
  }

  // --- MODO AUTOMATICO ---
  else if (cmd == "/autoon") {
    resultado.reconocido = true;
    if (!mediciones.potenciometroValido) {
      resultado.ejecutado = false;
      resultado.respuesta = "Error: Potenciómetro inválido. No se puede activar.";
    } else {
      if (Actuadores::activarAutomatico(mediciones.potenciometroV)) {
        Actuadores::actualizar(mediciones);
        EstadoActuadores estado = Actuadores::obtenerEstado();
        resultado.ejecutado = true;
        resultado.respuesta = "Modo automático activado. Línea base: " + String(estado.lineaBaseV, 2) + " V.";
      } else {
        resultado.ejecutado = false;
        resultado.respuesta = "Error: Voltaje fuera de rango.";
      }
    }
  }
  else if (cmd == "/autooff") {
    resultado.reconocido = true;
    Actuadores::desactivarAutomatico();
    Actuadores::actualizar(mediciones);
    resultado.ejecutado = true;
    resultado.respuesta = "Modo automático desactivado. Control manual restaurado.";
  }

  // --- CONFIGURACION DE UMBRAL ---
  else if (cmd == "/umbral" || cmd.startsWith("/umbral ")) {
    resultado.reconocido = true;
    if (cmd == "/umbral") {
      Actuadores::configurarUmbral(NAN);
      Actuadores::actualizar(mediciones);
      resultado.ejecutado = false;
      resultado.respuesta = "Falta valor. Use /umbral <1.0 a 1.5>. Se restableció a 0.5 V.";
    } else {
      String arg = cmd.substring(8);
      arg.trim();
      char* endPtr = nullptr;
      float val = strtof(arg.c_str(), &endPtr);

      if (arg.length() > 0 && endPtr != arg.c_str() && *endPtr == '\0' && !isnan(val) && !isinf(val)) {
        if (Actuadores::configurarUmbral(val)) {
          Actuadores::actualizar(mediciones);
          resultado.ejecutado = true;
          resultado.respuesta = "Umbral configurado a " + String(val, 2) + " V.";
        } else {
          Actuadores::actualizar(mediciones);
          resultado.ejecutado = false;
          resultado.respuesta = "Fuera de rango [1.0, 1.5] V. Restablecido a 0.5 V.";
        }
      } else {
        Actuadores::configurarUmbral(NAN);
        Actuadores::actualizar(mediciones);
        resultado.ejecutado = false;
        resultado.respuesta = "Formato inválido. Use punto decimal (ej: /umbral 1.2). Restablecido a 0.5 V.";
      }
    }
  }

  // --- VISTAS OLED ---
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
    resultado.respuesta = "Display OLED: Vista de Potenciómetro seleccionada.";
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
    resultado.respuesta = "Error: Comando de display desconocido (" + cmd + ").";
  }

  // --- TELEMETRIA ---
  else if (cmd == "/thingup") {
    resultado.reconocido = true;
    ResultadoEnvio envio = Telemetria::enviarAhora(mediciones, ahoraMs);
    switch (envio.estado) {
      case EstadoEnvio::Enviado:
        resultado.ejecutado = true;
        resultado.respuesta = "Datos subidos con éxito a ThingSpeak (Registro #" + String(envio.idRegistro) + ").";
        break;
      case EstadoEnvio::SinWifi:
        resultado.ejecutado = false;
        resultado.respuesta = "Error: Wi-Fi desconectado.";
        break;
      case EstadoEnvio::DatosInvalidos:
        resultado.ejecutado = false;
        resultado.respuesta = "Error: Lectura DHT inválida.";
        break;
      case EstadoEnvio::IntervaloNoCumplido:
        resultado.ejecutado = false;
        resultado.respuesta = "Espere el intervalo mínimo de 20s de ThingSpeak.";
        break;
      case EstadoEnvio::ErrorHttp:
        resultado.ejecutado = false;
        resultado.respuesta = "Error HTTP (" + String(envio.codigoHttp) + ").";
        break;
      default:
        resultado.ejecutado = false;
        resultado.respuesta = "No se pudo realizar el envío a ThingSpeak.";
        break;
    }
  }

  // --- DESCONOCIDO ---
  else {
    resultado.reconocido = false;
    resultado.ejecutado = false;
    resultado.respuesta = "Comando no reconocido. Toque 🔙 Menú Principal para ver las opciones.";
  }

  // Historial OLED: solo comandos operativos de Telegram exitosos (que empiezan con '/')
  if (origen == OrigenComando::Telegram && resultado.ejecutado && cmd.startsWith("/") && cmd != "/start" && cmd != "/menu" && cmd != "/ayuda" && cmd != "/help") {
    Pantalla::registrarComandoExitoso(cmd);
  }

  return resultado;
}

} // namespace Comandos
