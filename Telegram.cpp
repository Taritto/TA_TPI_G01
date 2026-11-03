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
  const unsigned long INTERVALO_TELEGRAM_MS = 500;
  const int MAX_MENSAJES_POR_CICLO = 5;

  enum class MenuNivel { Principal, Monitoreo, Leds, Auto, Oled };
  MenuNivel menuActual = MenuNivel::Principal;

  // Constructor dinámico del teclado según el submenú y el estado de actuadores
  String generarTecladoJson(MenuNivel nivel) {
    EstadoActuadores estado = Actuadores::obtenerEstado();

    if (nivel == MenuNivel::Principal) {
      return "["
        "[\"📋 Monitoreo General\", \"💡 Control de LEDs\"],"
        "[\"⚙️ Modo Automático\", \"🖥️ Pantalla OLED\"]"
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
      // Si está encendido ofrece apagar; si está apagado ofrece encender
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
      // Si está en automático ofrece desactivar; si está en manual ofrece activar
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

    return "[]";
  }
}

namespace Telegram {

void iniciar(const char* token) {
  clienteTelegram.setInsecure();

  if (bot != nullptr) {
    delete bot;
  }
  bot = new UniversalTelegramBot(token, clienteTelegram);
  ultimaConsultaMs = 0;
  menuActual = MenuNivel::Principal;
}

void actualizar(const Mediciones& mediciones, unsigned long ahoraMs) {
  if (bot == nullptr) {
    return;
  }

  if (!Conectividad::estaConectado()) {
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

      // 1. Detección de navegación entre submenús
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

      // 2. Ejecución lógica
      ResultadoComando resultado = Comandos::ejecutar(
        texto,
        OrigenComando::Telegram,
        mediciones,
        ahoraMs
      );

      // 3. Envío de respuesta con el teclado dinámico del submenú actual
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