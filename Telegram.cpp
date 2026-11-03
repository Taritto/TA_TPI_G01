#include "Telegram.h"
#include "Comandos.h"
#include "Conectividad.h"
#include "Actuadores.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

namespace {
  WiFiClientSecure clienteTelegram;
  UniversalTelegramBot* bot = nullptr;

  unsigned long ultimaConsultaMs = 0;
  const unsigned long INTERVALO_TELEGRAM_MS = 50; // Agilidad máxima
  const int MAX_MENSAJES_POR_CICLO = 5;

  String chatIdDestino = ""; // Almacena el chat activo para enviar alertas push

  enum class MenuNivel { Principal, Monitoreo, Leds, Auto, Oled, Ayuda };
  MenuNivel menuActual = MenuNivel::Principal;

  String generarTecladoJson(MenuNivel nivel) {
    EstadoActuadores estado = Actuadores::obtenerEstado();

    if (nivel == MenuNivel::Principal) {
      return "["
        "[\"📋 Monitoreo General\", \"💡 Control de LEDs\"],"
        "[\"⚙️ Modo Automático\", \"🖥️ Pantalla OLED\"],"
        "[\"📖 Guía de Comandos\"]"
      "]";
    }
    else if (nivel == MenuNivel::Monitoreo) {
      return "["
        "[\"/estado\", \"/ping\"],"
        "[\"/thingup\"],"
        "[\"🔙 Menú Principal\"]"
      "]";
    }
    else if (nivel == MenuNivel::Leds) {
      String botonVerde = estado.ledVerde ? "\"/ledverdeoff\"" : "\"/ledverdeon\"";
      String botonAzul;
      if (estado.automatico) {
        botonAzul = "\"(LED Azul en Modo Automático)\"";
      } else {
        botonAzul = estado.ledAzul ? "\"/ledazuloff\"" : "\"/ledazulon\"";
      }

      return "["
        "[" + botonVerde + "],"
        "[" + botonAzul + "],"
        "[\"🔙 Menú Principal\"]"
      "]";
    }
    else if (nivel == MenuNivel::Auto) {
      String botonAuto = estado.automatico ? "\"/autooff\"" : "\"/autoon\"";
      return "["
        "[" + botonAuto + "],"
        "[\"🔙 Menú Principal\"]"
      "]";
    }
    else if (nivel == MenuNivel::Oled) {
      return "["
        "[\"/displayled\", \"/displaypot\"],"
        "[\"/displaydht\"],"
        "[\"🔙 Menú Principal\"]"
      "]";
    }
    else if (nivel == MenuNivel::Ayuda) {
      return "["
        "[\"🔙 Menú Principal\"]"
      "]";
    }

    return "[]";
  }
}

namespace Telegram {

void iniciar(const char* token) {
  clienteTelegram.setInsecure();
  clienteTelegram.setTimeout(2500); // Evita bloqueos largos de socket

  if (bot != nullptr) {
    delete bot;
  }

  // 1. Crear el objeto en memoria
  bot = new UniversalTelegramBot(token, clienteTelegram);

  // 2. Activar Long Polling
  bot->longPoll = 2;

  ultimaConsultaMs = 0;
  chatIdDestino = "";
  menuActual = MenuNivel::Principal;
}

bool hayChatRegistrado() {
  return (chatIdDestino.length() > 0);
}

bool enviarAlerta(const String& mensaje) {
  if (bot == nullptr || chatIdDestino.length() == 0) {
    return false;
  }
  if (!Conectividad::estaConectado()) {
    return false;
  }
  return bot->sendMessage(chatIdDestino, mensaje, "");
}

void actualizar(const Mediciones& mediciones, unsigned long ahoraMs) {
  if (bot == nullptr || !Conectividad::estaConectado()) {
    return;
  }

  if (ahoraMs - ultimaConsultaMs < INTERVALO_TELEGRAM_MS) {
    return;
  }
  ultimaConsultaMs = ahoraMs;

  int cantidadMensajes = bot->getUpdates(bot->last_message_received + 1);
  int mensajesProcesados = 0;

  while (cantidadMensajes > 0 && mensajesProcesados < MAX_MENSAJES_POR_CICLO) {
    for (int i = 0; i < cantidadMensajes; i++) {
      String chatId = bot->messages[i].chat_id;
      String texto = bot->messages[i].text;

      chatIdDestino = chatId;

      // Navegación de menús
      if (texto == "/start" || texto == "🔙 Menú Principal" || texto == "/menu") {
        menuActual = MenuNivel::Principal;
      }
      else if (texto == "📋 Monitoreo General") {
        menuActual = MenuNivel::Monitoreo;
      }
      else if (texto == "💡 Control de LEDs") {
        menuActual = MenuNivel::Leds;
      }
      else if (texto == "⚙️ Modo Automático") {
        menuActual = MenuNivel::Auto;
      }
      else if (texto == "🖥️ Pantalla OLED") {
        menuActual = MenuNivel::Oled;
      }
      else if (texto == "📖 Guía de Comandos" || texto == "/ayuda" || texto == "/help") {
        menuActual = MenuNivel::Ayuda;
      }

      // Ejecución lógica
      ResultadoComando resultado = Comandos::ejecutar(
        texto,
        OrigenComando::Telegram,
        mediciones,
        ahoraMs
      );

      // Respuesta con el teclado del submenú
      if (resultado.respuesta.length() > 0) {
        String teclado = generarTecladoJson(menuActual);
        bot->sendMessageWithReplyKeyboard(chatId, resultado.respuesta, "", teclado, true, false);
      }

      mensajesProcesados++;
    }

    if (mensajesProcesados < MAX_MENSAJES_POR_CICLO) {
      cantidadMensajes = bot->getUpdates(bot->last_message_received + 1);
    } else {
      break;
    }
  }
}

} // namespace Telegram