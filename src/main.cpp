// ======================================================
// TRABAJO PRACTICO INTEGRADOR - TA 4K01
// Sistema embebido ESP32
// ======================================================

// ======================================================
// 1. INCLUSION DE LIBRERIAS
// ======================================================

#include "ThingSpeakManager.h" //Incluimos la libreria con las funcionalidades en ThingSpeak.
#include "WiFiManager.h" //Incluimos la librería con las funcionalidades de conección WiFi.

// ======================================================
// 3. DECLARACION DE OBJETOS
// ======================================================

ThingSpeakManager tsManager("NAAK139LTSLRUOGD"); // Instanciamos el objeto de la librería.

WiFiManager wifiManager("Fibertel WiFi458 2.4GHz", "1142792048"); //Intanciamos el objeto de la librería y le pasamos nuestra red y la contraseña.

// ======================================================
// 5. SETUP()
// ======================================================

void setup()
{
  

  wifiManager.conectar(); // Reemplazamos la función conectarWiFi() que eliminamos abajo por la llamada al metodo del objeto.

  
}



// ======================================================
// CONEXION WIFI
// ======================================================
// Pa la librería parguela

// ======================================================
// PROCESAMIENTO DE MENSAJES TELEGRAM
// ======================================================

void procesarMensajesTelegram(int cantidadMensajes)
{

    if (texto == "/start")
    {
      String respuesta = "SISTEMA DE MONITOREO ESP32\n\n";
      respuesta += "COMANDOS GENERALES\n";
      respuesta += "/estado - Consultar estado general\n";
      respuesta += "/umbral <valor> - Configurar umbral\n";
      respuesta += "/autoon - Activar modo automatico\n";
      respuesta += "/autooff - Activar modo manual\n\n";
      respuesta += "CONTROL MANUAL DE LEDS\n";
      respuesta += "/ledverdeon - Encender LED verde\n";
      respuesta += "/ledverdeoff - Apagar LED verde\n";
      respuesta += "/ledazulon - Encender LED azul\n";
      respuesta += "/ledazuloff - Apagar LED azul\n\n";
      respuesta += "DISPLAY OLED\n";
      respuesta += "/displayled\n";
      respuesta += "/displaypot\n";
      respuesta += "/displaydht\n";
      bot.sendMessage(chat_id, respuesta, "");
    }
    else if (texto == "/ping")
    {
      bot.sendMessage(chat_id, "ESP32 funcionando correctamente!", "");
    }
    // ENVIO MANUAL A THINGSPEAK
    else if (texto == "/thingup")
    {
      tsManager.enviarDatos(temperatura, humedad); //Reemplazamos aqui la función enviarDatosThingSpeak previa por el metodo del objeto tsManager
      bot.sendMessage(
          chat_id,
          "Se realizo el intento de envio de datos a ThingSpeak. Revise el Monitor Serie para conocer el resultado.",
          "");
    }
    else if (texto == "/ledverdeon")
    {
      encenderLedVerde();
      bot.sendMessage(chat_id, "LED verde encendido.", "");
    }
    else if (texto == "/ledverdeoff")
    {
      apagarLedVerde();
      bot.sendMessage(chat_id, "LED verde apagado.", "");
    }
    else if (texto == "/ledazulon")
    {
      if (!modoAutomatico)
      {
        encenderLedAzul();
        bot.sendMessage(chat_id, "LED azul encendido.", "");
      }
      else
      {
        bot.sendMessage(chat_id, "Desactive el modo automatico con /autooff para controlar manualmente el LED azul.", "");
      }
    }
    else if (texto == "/ledazuloff")
    {
      if (!modoAutomatico)
      {
        apagarLedAzul();
        bot.sendMessage(chat_id, "LED azul apagado.", "");
      }
      else
      {
        bot.sendMessage(chat_id, "El LED azul esta siendo controlado por el modo automatico. Utilice /autooff para desactivarlo.", "");
      }
    }
    else if (texto == "/estado")
    {
      String respuesta = "ESTADO DEL SISTEMA\n\n";
      respuesta += "Temperatura: " + String(temperatura) + " C\n";
      respuesta += "Humedad: " + String(humedad) + " %\n";
      respuesta += "Potenciometro: " + String(voltajePotenciometro) + " V\n";
      respuesta += "LED Verde: " + String(estadoLedVerde ? "ENCENDIDO" : "APAGADO") + "\n";
      respuesta += "LED Azul: " + String(estadoLedAzul ? "ENCENDIDO" : "APAGADO") + "\n";
      respuesta += "Modo: " + String(modoAutomatico ? "AUTOMATICO" : "MANUAL");
      bot.sendMessage(chat_id, respuesta, "");
    }
    else if (texto == "/autoon")
    {
      activarModoAutomatico();
      bot.sendMessage(chat_id, "Modo automatico activado correctamente.", "");
    }
    else if (texto == "/autooff")
    {
      desactivarModoAutomatico();
      bot.sendMessage(chat_id, "Modo manual activado correctamente.", "");
    }
    else if (texto.startsWith("/umbral "))
    {
      float nuevoUmbral = texto.substring(8).toFloat();
      if (nuevoUmbral >= 1.0 && nuevoUmbral <= 1.5)
      {
        umbral = nuevoUmbral;
        bot.sendMessage(chat_id, "Umbral actualizado: " + String(umbral, 2) + " V", "");
      }
      else
      {
        bot.sendMessage(chat_id, "Valor invalido. El umbral debe estar entre 1 y 1.5 V.", "");
      }
    }
    else if (texto == "/displayled")
    {
      mostrarEstadoLed();
      bot.sendMessage(chat_id, "Estado de los LEDs mostrado en OLED.", "");
    }
    else if (texto == "/displaypot")
    {
      mostrarEstadoPot();
      bot.sendMessage(chat_id, "Estado del potenciometro mostrado en OLED.", "");
    }
    else if (texto == "/displaydht")
    {
      mostrarEstadoDHT();
      bot.sendMessage(chat_id, "Estado del DHT mostrado en OLED.", "");
    }
    // COMANDO DESCONOCIDO
    else
    {

      bot.sendMessage(chat_id, "Comando no reconocido.", "");
    }
  
}

// CONSULTA DE MENSAJES TELEGRAM
void consultarTelegram()
{
  if (millis() - tiempoUltimaConsulta >= INTERVALO_TELEGRAM)
  {
    int cantidadMensajes = bot.getUpdates(bot.last_message_received + 1);

    while (cantidadMensajes)
    {
      procesarMensajesTelegram(cantidadMensajes);
      cantidadMensajes = bot.getUpdates(bot.last_message_received + 1);
    }

    tiempoUltimaConsulta = millis();
  }
  /*
    // ENVIO AUTOMATICO A THINGSPEAK
  if (millis() - tiempoUltimoEnvioThingSpeak >= INTERVALO_THINGSPEAK) {

    tsManager.enviarDatos(temperatura, humedad);

  }
  */
}




// ======================================================
// ENVIO DE DATOS A THINGSPEAK
// ======================================================
// Pa la librería parguela.