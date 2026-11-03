#include "Telegram.h"
#include "Comandos.h"
#include "Conectividad.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

namespace {
  // Objetos privados al archivo (encapsulados)
  WiFiClientSecure clienteTelegram;
  UniversalTelegramBot* bot = nullptr;

  unsigned long ultimaConsultaMs = 0;
  const unsigned long INTERVALO_TELEGRAM_MS = 1000;
  const int MAX_MENSAJES_POR_CICLO = 5; // Evita que una avalancha de mensajes bloquee el loop()
}

namespace Telegram {

void iniciar(const char* token) {
  // Desactiva validación estricta de certificados CA para el entorno embebido
  clienteTelegram.setInsecure();

  if (bot != nullptr) {
    delete bot;
  }
  bot = new UniversalTelegramBot(token, clienteTelegram);
  ultimaConsultaMs = 0;
}

void actualizar(const Mediciones& mediciones, unsigned long ahoraMs) {
  if (bot == nullptr) {
    return;
  }

  // 1. Verificación de enlace Wi-Fi antes de abrir conexiones de socket seguro
  if (!Conectividad::estaConectado()) {
    return;
  }

  // 2. Control de tiempo no bloqueante
  if (ahoraMs - ultimaConsultaMs < INTERVALO_TELEGRAM_MS) {
    return;
  }
  ultimaConsultaMs = ahoraMs;

  // 3. Sondeo de nuevos mensajes avanzando el ID recibido
  int cantidadMensajes = bot->getUpdates(bot->last_message_received + 1);

  // 4. Procesamiento acotado por vuelta
  int mensajesProcesados = 0;
  while (cantidadMensajes > 0 && mensajesProcesados < MAX_MENSAJES_POR_CICLO) {
    for (int i = 0; i < cantidadMensajes; i++) {
      String chatId = bot->messages[i].chat_id;
      String texto = bot->messages[i].text;

      // Delegación de la orden al módulo Comandos
      ResultadoComando resultado = Comandos::ejecutar(
        texto,
        OrigenComando::Telegram,
        mediciones,
        ahoraMs
      );


     // Envío de la respuesta generada con teclado de botones interactivos
      if (resultado.respuesta.length() > 0) {
                if (texto == "/start") {
          // Matriz de botones
          String tecladoJson = "{"
            "\"keyboard\": ["
              "[\"/estado\", \"/ping\"],"
              "[\"/ledverdeon\", \"/ledverdeoff\"],"
              "[\"/ledazulon\", \"/ledazuloff\"],"
              "[\"/autoon\", \"/autooff\"],"
              "[\"/displayled\", \"/displaypot\", \"/displaydht\"],"
              "[\"/thingup\"]"
            "],"
            "\"resize_keyboard\": true,"
            "\"is_persistent\": true,"
            "\"input_field_placeholder\": \"⛔ Seleccione una opción abajo\""
          "}";

          bot->sendMessageWithReplyKeyboard(chatId, resultado.respuesta, "", tecladoJson, false, false);
        } else {
          bot->sendMessage(chatId, resultado.respuesta, "");
        }
      }

      mensajesProcesados++;
    }

    // Consulta si quedaron mensajes pendientes solo si no superamos el límite del ciclo
    if (mensajesProcesados < MAX_MENSAJES_POR_CICLO) {
      cantidadMensajes = bot->getUpdates(bot->last_message_received + 1);
    } else {
      break;
    }
  }
}

} // namespace Telegram