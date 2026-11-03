# Plan de implementación e integración del TPI

Fecha: 2 de noviembre de 2026.

## 1. Alcance y punto de partida

Este documento define una arquitectura propuesta para repartir el trabajo entre los cinco integrantes que se incorporan al desarrollo. El compañero que escribió la base coordina la integración. Los nombres y firmas siguientes son el contrato a implementar: **todavía no existen en el código**.

Se toma como referencia la **consigna 01**, porque `tpTA.ino` implementa umbral, modo automático e historial de tres comandos. La consigna 02 tiene requisitos diferentes; no se agregan botones inline, relé temporizado ni alertas por voltaje a este plan. Confirmar con la cátedra qué enunciado corresponde antes de cerrar la entrega.

Fuentes revisadas: `tpTA.ino` del repositorio y los documentos `TA4k01-TPI - 01.pdf` y `TA4k01-TPI - 02.pdf`. Las decisiones de arquitectura de este documento son propuestas del equipo, no exigencias textuales del enunciado.

La base tiene un solo `.ino`. Ya utiliza bibliotecas externas, pero falta separar responsabilidades en bibliotecas propias. Hay que preservar su funcionamiento mientras se extrae cada parte y se corrigen los pendientes identificados.

## 2. Responsables y propiedad de archivos

| Responsable | Parte | Archivos que le pertenecen |
|---|---|---|
| Integrante 1 | Sensores | `Sensores.h`, `Sensores.cpp` |
| Integrante 2 | Actuadores y control automático | `Actuadores.h`, `Actuadores.cpp` |
| Integrante 3 | OLED e historial | `Pantalla.h`, `Pantalla.cpp` |
| Integrante 4 | Telegram e interpretación común de comandos | `Telegram.h`, `Telegram.cpp`, `Comandos.h`, `Comandos.cpp` |
| Integrante 5 | Wi-Fi y ThingSpeak | `Conectividad.h`, `Conectividad.cpp`, `Telemetria.h`, `Telemetria.cpp` |
| Compañero autor de la base / coordinador | Integración, configuración y tipos compartidos | `tpTA.ino`, `Tipos.h`, `Config.h`, `Secretos.example.h`, `.gitignore`, documentación de ejecución y archivos del proyecto Wokwi |

`Secretos.h` será local a cada equipo y no se versionará. El coordinador prepara su plantilla; cada integrante completa sus valores locales.

Para la primera separación, los `.h` y `.cpp` estarán junto al `.ino`. No hace falta introducir PlatformIO para cumplir este plan. El coordinador verificará la estructura de carpeta y nombre del sketch admitida por el entorno elegido, porque actualmente la carpeta del repositorio y el `.ino` tienen nombres distintos. Si se cambia de estructura, debe hacerlo una sola persona y avisar a todos antes de que adapten sus ramas.

**Reglas de propiedad:**

- Cada integrante modifica sus archivos. Si necesita un cambio en un contrato común, lo acuerda con el coordinador y con sus consumidores.
- Solo el coordinador modifica el `.ino`, `Tipos.h` y `Config.h` durante la integración.
- Cada dueño mueve la lógica desde la base a sus archivos; la eliminación de la versión vieja del `.ino` la hace el coordinador al conectar ese módulo. No deben quedar dos implementaciones activas controlando el mismo recurso.
- Nadie redefine estructuras compartidas ni accede por `extern` a las antiguas variables globales.
- No se incluyen archivos `.cpp` desde otros archivos. Se incluyen sus `.h`.

## 3. Acuerdos comunes que deben cerrarse primero

### 3.1 Tipos de datos compartidos

El coordinador crea `Tipos.h` antes de repartir las implementaciones. Debe incluir `Arduino.h` y definir exactamente estos tipos:

```cpp
#pragma once
#include <Arduino.h>
#include <math.h>

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
  NoCorresponde,       // Solo actualizar(): aún no vence el envío periódico.
  Enviado,
  SinWifi,
  DatosInvalidos,
  IntervaloNoCumplido,
  ErrorHttp,
  RechazadoServidor
};

struct ResultadoEnvio {
  EstadoEnvio estado = EstadoEnvio::NoCorresponde;
  int codigoHttp = 0; // 0 si no se intentó HTTP; negativo si lo devuelve el cliente.
  long idRegistro = 0; // > 0 únicamente cuando ThingSpeak confirma el registro.
};

struct ResultadoComando {
  bool reconocido = false;
  bool ejecutado = false;
  String respuesta;
};
```

Se incluye `math.h` explícitamente para disponer de `NAN` y de las comprobaciones de valores no finitos.

Las mediciones son una copia del último muestreo, no una orden de leer hardware. Los estados de validez son obligatorios: un cero no significa error. Ante un intento de lectura fallido se marca el dato inválido; no se presenta una lectura anterior como si acabara de obtenerse. Las marcas de tiempo corresponden al último intento de muestreo y no se usan para decidir validez por sí solas.

### 3.2 Configuración común

`Config.h` contiene constantes con estos nombres acordados:

| Constante | Valor inicial / significado |
|---|---|
| `DHT_PIN`, `DHT_TIPO` | GPIO 33; tipo DHT22, corrigiendo el DHT11 actual |
| `POT_PIN` | GPIO 32 |
| `LED_VERDE_PIN`, `LED_AZUL_PIN` | GPIO 23 y GPIO 2 |
| `OLED_SDA`, `OLED_SCL` | GPIO 21 y GPIO 22 |
| `OLED_ANCHO`, `OLED_ALTO`, `OLED_DIRECCION` | 128, 64 y `0x3C` |
| `INTERVALO_DHT_MS` | 2000 ms como intervalo inicial del proyecto |
| `INTERVALO_POT_MS` | 100 ms como intervalo inicial del proyecto |
| `INTERVALO_TELEGRAM_MS` | 1000 ms, conservando el criterio actual |
| `INTERVALO_THINGSPEAK_MS` | 20000 ms, conservando el criterio actual |
| `INTERVALO_RECONEXION_MS` | 10000 ms como política inicial del proyecto |
| `UMBRAL_DEFECTO_V` | 0.5 V |
| `UMBRAL_MIN_V`, `UMBRAL_MAX_V` | 1.0 y 1.5 V, inclusive |

Los intervalos son decisiones de implementación; no todos aparecen como valores obligatorios en la consigna. Para ThingSpeak, verificar que el intervalo configurado sea compatible con el canal utilizado.

`Secretos.example.h` declara `WIFI_SSID`, `WIFI_PASSWORD`, `BOT_TOKEN` y `THINGSPEAK_API_KEY` con valores ficticios. Cada equipo copia esa plantilla como `Secretos.h`. Las credenciales reales que ya quedaron en el código y en su historial deben reemplazarse; agregar `.gitignore` no elimina la exposición histórica.

### 3.3 Convenciones de las funciones

- Las interfaces públicas usan `namespace` con el nombre del módulo y se declaran en su `.h` con `#pragma once`.
- Los objetos físicos y de red se definen una sola vez dentro del `.cpp` dueño, con alcance privado.
- `void` significa que no devuelve valor; las funciones que pueden ser rechazadas devuelven `bool` o un resultado explícito.
- `const Mediciones&` y `const EstadoActuadores&` reciben datos de solo lectura, sin copiar toda la estructura.
- `ahoraMs` es una captura de `millis()`. Las esperas se verifican con resta unsigned: `ahoraMs - anteriorMs >= intervaloMs`.
- No agregar `delay()` ni bucles de espera indefinida en las funciones periódicas. Las llamadas HTTP/Telegram pueden ser síncronas; deben tener tiempos de espera acotados. Este plan no promete red completamente asíncrona.
- Ningún módulo vuelve a llamar a `setup()` o `loop()` ni requiere que otro conozca sus variables internas.

## 4. Integrante 1: módulo Sensores

### Responsabilidad y código a extraer

Extraer el objeto `DHT dht`, las variables de mediciones, `dht.begin()` y las lecturas que hoy están al principio de `loop()`. Este módulo es el único que lee DHT y ADC. Utiliza DHT y las funciones analógicas del entorno ESP32.

### Interfaz pública: `Sensores.h`

```cpp
namespace Sensores {
  void iniciar();
  void actualizar(unsigned long ahoraMs);
  Mediciones obtenerMediciones();
}
```

| Función | Parámetros | Resultado y comportamiento |
|---|---|---|
| `iniciar()` | Ninguno; usa `Config.h` | Inicializa el DHT22 y el estado interno. Deja ambos datos inválidos hasta que se muestreen. |
| `actualizar(ahoraMs)` | Tiempo actual en ms | `void`. Muestrea cada dispositivo solo cuando vence su intervalo. Debe permitir una primera lectura del potenciómetro en la primera llamada; el DHT respeta su tiempo de arranque. |
| `obtenerMediciones()` | Ninguno | Devuelve por valor una instantánea `Mediciones`. No lee hardware ni espera. |

### Comportamiento a implementar

- Temperatura en grados Celsius, humedad en porcentaje y potenciómetro en voltios.
- Mantener ADC crudo y conversión de milivoltios a voltios mediante el criterio de la base; comprobar el rango esperado de 0 a 3.3 V en el montaje elegido.
- No transformar `NaN` en cero ni marcar válido un dato no finito.
- Leer DHT y potenciómetro con temporizadores independientes.
- Evitar imprimir en cada vuelta del `loop()`; los mensajes de diagnóstico deben tener una frecuencia acotada.

### Relación con otros módulos y con el `.ino`

No depende de Pantalla, Telegram, Actuadores ni Telemetria. El `.ino` llama a `iniciar()` una vez y a `actualizar()` en cada vuelta. Luego obtiene la copia que reciben los demás. Cambiar nombres, unidades o significado de validez requiere avisar a todos.

### Pruebas para darlo por terminado

1. Lecturas normales del DHT22 y del potenciómetro en sus extremos y en una posición intermedia.
2. DHT desconectado o lectura fallida: `dhtValido == false`, sin impedir el potenciómetro.
3. Consultar repetidamente `obtenerMediciones()` no genera muestreos adicionales.

## 5. Integrante 2: módulo Actuadores

### Responsabilidad y código a extraer

Extraer `pinMode` y escrituras de LEDs, estados de LEDs, `modoAutomatico`, `umbral`, `lineaBase` y todas las funciones de control manual/automático. Es el único módulo autorizado a escribir los GPIO de LEDs.

### Interfaz pública: `Actuadores.h`

```cpp
namespace Actuadores {
  void iniciar();
  bool fijarLedVerde(bool encendido);
  bool fijarLedAzulManual(bool encendido);
  bool activarAutomatico(float voltajeActualV);
  void desactivarAutomatico();
  bool configurarUmbral(float valorV);
  void actualizar(const Mediciones& mediciones);
  EstadoActuadores obtenerEstado();
}
```

| Función | Parámetros | Resultado y comportamiento |
|---|---|---|
| `iniciar()` | Ninguno | `void`. Configura GPIO, apaga LEDs y establece manual, línea base 0 y umbral 0.5 V. |
| `fijarLedVerde(encendido)` | `true` enciende; `false` apaga | `true` si se aceptó, incluso si ya tenía ese estado. Disponible en ambos modos. |
| `fijarLedAzulManual(encendido)` | Estado solicitado | `false` si está en automático y no cambia el LED; `true` si se acepta. |
| `activarAutomatico(voltajeActualV)` | Lectura válida del potenciómetro en V | `true`: captura la línea base, activa automático y evalúa el LED con esa lectura. `false` ante valor no finito o fuera de 0 a 3.3 V, sin cambiar estado. |
| `desactivarAutomatico()` | Ninguno | `void`. Vuelve a manual y apaga el azul, conservando la conducta de la base. |
| `configurarUmbral(valorV)` | Diferencia de voltaje solicitada | `true` si pertenece a [1.0, 1.5]; en otro caso restaura 0.5 V y devuelve `false`. Rechaza también valores no finitos. |
| `actualizar(mediciones)` | Copia de mediciones por referencia constante | `void`. Si está en automático y el potenciómetro es válido, enciende azul cuando `potenciometroV > lineaBaseV + umbralV`; en igualdad o por debajo lo apaga. Con lectura inválida apaga azul como política del proyecto. |
| `obtenerEstado()` | Ninguno | Devuelve por valor `EstadoActuadores`; sin efectos secundarios. |

### Decisiones y casos importantes

- La fórmula `línea base + umbral` conserva la interpretación del código existente. La consigna no desarrolla toda la fórmula: validarla con la cátedra si hay dudas.
- Cada `/autoon` aceptado captura nuevamente la lectura actual, aunque el modo ya estuviera activo.
- Si línea base más umbral supera 3.3 V, no se modifica silenciosamente el límite: el azul no se encenderá dentro del rango normal. Informar esa situación en la documentación/defensa.
- La regla que impide control manual del azul en automático vive aquí. Debe aplicarse igual desde Telegram y desde serie.
- Después de cambiar el umbral o el modo, el despachador llama a `actualizar(mediciones)` antes de construir respuestas de estado que necesiten reflejar ese cambio.
- No guardar duplicados de `estadoLedAzul` o `modoAutomatico` en otros módulos.

### Relación con otros módulos y con el `.ino`

No lee sensores directamente: recibe `Mediciones`. Comandos llama a las operaciones manuales y de configuración. Pantalla y Telegram consultan el estado por la interfaz. El `.ino` inicia el módulo y ejecuta su actualización periódica.

### Pruebas para darlo por terminado

1. Encendido y apagado manual de ambos LEDs.
2. Línea base 1.0 V, umbral 1.0 V: azul apagado con 2.0 V y encendido con un valor mayor a 2.0 V.
3. Umbral 1.2 V aceptado; luego 0.8 V rechazado y umbral restaurado a 0.5 V.
4. Órdenes manuales del azul rechazadas en automático tanto por serie como por Telegram.
5. Desactivar automático apaga el azul; lectura inválida no provoca una activación.

## 6. Integrante 3: módulo Pantalla

### Responsabilidad y código a extraer

Extraer el objeto OLED, inicialización de `Wire` y del display, `mostrarEstadoLed`, `mostrarEstadoPot`, `mostrarEstadoDHT`, `guardarComando`, `mostrarHistorialComandos` y `mostrarHistorialEnPantalla`. Este módulo posee el búfer de tres comandos y el único objeto `Adafruit_SSD1306`.

### Interfaz pública: `Pantalla.h`

```cpp
namespace Pantalla {
  bool iniciar();
  void seleccionarVista(VistaPantalla vista);
  void mostrarErrorComando(const String& comando);
  void registrarComandoExitoso(const String& comando);
  void actualizar(const Mediciones& mediciones,
                  const EstadoActuadores& estado,
                  unsigned long ahoraMs);
}
```

| Función | Parámetros | Resultado y comportamiento |
|---|---|---|
| `iniciar()` | Ninguno | Inicializa I2C/OLED y muestra inicio. `true` si está disponible; `false` si falla. |
| `seleccionarVista(vista)` | `Inicio`, `Leds`, `Potenciometro` o `Dht` | `void`. Cambia la vista persistente. Para errores usar `mostrarErrorComando`; pasar `Error` directamente no crea un mensaje nuevo. |
| `mostrarErrorComando(comando)` | Comando `/display...` no reconocido | `void`. Guarda un mensaje breve y selecciona `Error`, sin agregar el comando al historial. |
| `registrarComandoExitoso(comando)` | Texto de un comando Telegram ejecutado con éxito | `void`. Desplaza el historial: índice 0 más reciente, hasta tres entradas. No cambia la vista. |
| `actualizar(mediciones, estado, ahoraMs)` | Datos actuales, estado de actuadores y tiempo | `void`. Dibuja vista e historial. Debe refrescar ante cambios y puede limitar el refresco periódico, por ejemplo a 200 ms; sin esperas activas. |

### Comportamiento a implementar

- Reservar espacio para el estado del componente y para tres líneas inferiores de historial. En 128x64, con texto de 8 píxeles de alto, una propuesta es cuatro líneas superiores, un separador y tres líneas de comandos.
- Acortar etiquetas y truncar comandos largos de forma explícita, sin permitir que el salto automático de línea invada otras filas.
- Mantener la vista elegida mientras llegan otros comandos. Por ejemplo, `/estado` actualiza el historial sin reemplazar la pantalla de potenciómetro por una pantalla que solo contiene historial.
- Un comando `/displayxyz` muestra error en OLED y Telegram, conservando los tres últimos comandos exitosos.
- La vista de error permanece hasta que otra orden de display seleccione una vista válida; este es un acuerdo del proyecto.
- Con datos inválidos mostrar `Sin lectura` o `Error`, nunca `nan` o un cero ficticio.
- Si falla la inicialización, las demás funciones deben retornar sin dibujar y el resto del sistema sigue funcionando. No usar el `while(true)` actual como política general.

### Relación con otros módulos y con el `.ino`

Pantalla no interpreta mensajes Telegram ni decide si fueron exitosos: Comandos se lo informa. No consulta sensores ni controla LEDs. El `.ino` la inicializa y le pasa mediciones y estado al final de cada vuelta. Comandos selecciona vistas o solicita un error; el dibujo se completa en `actualizar()`.

### Pruebas para darlo por terminado

1. Las tres vistas muestran datos correctos y el historial sin superposición.
2. Cuatro comandos exitosos conservan únicamente los últimos tres, en orden.
3. Comandos desconocidos o rechazados no se agregan al historial.
4. Un comando válido no relacionado con display conserva la vista elegida.
5. OLED ausente o DHT inválido no bloquean la aplicación.

## 7. Integrante 4: módulos Comandos y Telegram

### Por qué se separan en dos partes del mismo responsable

`Comandos` interpreta y ejecuta órdenes. `Telegram` transporta mensajes y respuestas. Esto permite que el monitor serie use las mismas reglas sin duplicar toda la cadena de `if/else`. Ambos módulos pertenecen al mismo integrante.

### 7.1 Interfaz pública: `Comandos.h`

```cpp
namespace Comandos {
  ResultadoComando ejecutar(const String& texto,
                            OrigenComando origen,
                            const Mediciones& mediciones,
                            unsigned long ahoraMs);
}
```

Parámetros: texto recibido, origen de la orden, instantánea de mediciones y tiempo actual. Devuelve `ResultadoComando` con estas reglas:

| Situación | `reconocido` | `ejecutado` | `respuesta` |
|---|---|---|---|
| Orden conocida y completada | `true` | `true` | Confirmación o datos solicitados |
| Orden conocida con argumento inválido o acción rechazada | `true` | `false` | Motivo y, si corresponde, valor aplicado por defecto |
| Orden desconocida | `false` | `false` | Ayuda breve; si empieza con `/display`, también solicita error en OLED |

`ejecutado` no significa que el envío de la respuesta por Telegram tuvo éxito: representa el resultado de la operación local. El historial registra órdenes ejecutadas correctamente, aunque luego falle la entrega de su confirmación.

### Tabla de órdenes y llamadas

| Orden normalizada | Validación y acción | Resultado esperado |
|---|---|---|
| `/start` | Construir ayuda con emojis, secciones y formato HTML o Markdown; incluir `/thingup` | Ayuda completa; `ejecutado = true` |
| `/estado` | Usar `mediciones` y consultar `Actuadores::obtenerEstado()` | Temperatura, humedad, voltaje, LEDs y modo; indicar datos inválidos |
| `/ledverdeon`, `/ledverdeoff` | `Actuadores::fijarLedVerde(true/false)` | Confirmación |
| `/ledazulon`, `/ledazuloff` | `Actuadores::fijarLedAzulManual(true/false)` | Confirmación o rechazo por modo automático |
| `/autoon` | Verificar `potenciometroValido`; llamar a `activarAutomatico(potenciometroV)` | Confirmación con línea base o error |
| `/autooff` | `Actuadores::desactivarAutomatico()` | Confirmación |
| `/umbral <valor>` | Parsear todo el argumento como número finito; llamar a `configurarUmbral(valor)` | Confirmación o rechazo indicando restauración a 0.5 V |
| `/thingup` | `Telemetria::enviarAhora(mediciones, ahoraMs)` | Éxito solo si devuelve `Enviado`; explicar sin Wi-Fi, espera, datos inválidos o error |
| `/displayled` | `Pantalla::seleccionarVista(VistaPantalla::Leds)` | Confirmación de selección |
| `/displaypot` | `Pantalla::seleccionarVista(VistaPantalla::Potenciometro)` | Confirmación de selección |
| `/displaydht` | `Pantalla::seleccionarVista(VistaPantalla::Dht)` | Confirmación de selección |
| `/display` o `/display...` desconocido | `Pantalla::mostrarErrorComando(texto)` | Error; no se registra como exitoso |
| `/ping` | Mantener como utilidad de la base | Respuesta breve |

Los controles manuales y `/ping` se conservan de la base. Para serie, aceptar los nombres actuales sin `/` y normalizarlos internamente a la misma orden. Los comandos serie no se agregan al historial Telegram.

No usar solamente `String::toFloat()` para validar: un texto con basura al final no debe aceptarse como número correcto. Acordar decimal con punto (`1.2`). Un `/umbral` sin número o con texto inválido restaura 0.5 V mediante `configurarUmbral(NAN)` y devuelve error; es una política explícita del proyecto. No requiere reiniciar la placa.

Después de una operación aceptada, llamar a `Actuadores::actualizar(mediciones)` si cambió el control. Si `origen == OrigenComando::Telegram` y `resultado.ejecutado == true`, registrar exactamente una vez el texto normalizado en Pantalla. Un `/thingup` fallido y una orden manual rechazada no ingresan al historial.

Una selección de display confirmada significa que se aceptó la vista; no garantiza presencia física del OLED. El `.ino` registra por serie si `Pantalla::iniciar()` falló.

### 7.2 Interfaz pública: `Telegram.h`

```cpp
namespace Telegram {
  void iniciar(const char* token);
  void actualizar(const Mediciones& mediciones, unsigned long ahoraMs);
}
```

| Función | Parámetros | Resultado y comportamiento |
|---|---|---|
| `iniciar(token)` | Token, con almacenamiento válido durante toda la aplicación | `void`. Crea/configura cliente seguro y bot. No espera indefinidamente a Wi-Fi. |
| `actualizar(mediciones, ahoraMs)` | Instantánea actual y tiempo | `void`. Si hay red y vence el intervalo, obtiene mensajes, llama a `Comandos::ejecutar` y envía cada respuesta al chat del mensaje correspondiente. |

### Código a extraer y precauciones

- Extraer objetos `WiFiClientSecure`/`UniversalTelegramBot`, consulta de actualizaciones y transporte de respuestas.
- Mover la interpretación desde `procesarMensajesTelegram()` y `procesarComandoSerie()` a `Comandos.cpp`.
- Telegram depende de Conectividad y Comandos. Comandos depende de Actuadores, Pantalla y Telemetria, además de `Tipos.h`; no depende de Telegram.
- Conservar avance del identificador de actualizaciones para no ejecutar otra vez el mismo mensaje. Limitar el trabajo por vuelta; no drenar una cola ilimitada en un `while` que impida volver al `loop()`.
- La instantánea de sensores es la de esa vuelta. Si se procesan varios mensajes juntos, usan esa misma instantánea; no obligar al DHT a leer otra vez por cada `/estado`.
- Usar formato de envío compatible con la ayuda generada y escapar contenido dinámico si corresponde.
- La base usa `setInsecure()`. Documentar esa condición si se conserva para pruebas; no describirla como validación de certificado. Su configuración pertenece únicamente a este módulo.

### Qué necesita saber del `.ino`

El `.ino` llama a `Telegram::iniciar(BOT_TOKEN)` una vez y a `actualizar()` en cada vuelta. Para serie, el coordinador conserva un pequeño lector de líneas y llama a `Comandos::ejecutar(..., OrigenComando::Serie, ...)`, imprimiendo `respuesta`. Ese lector debe acumular caracteres disponibles y procesar al recibir salto de línea, con longitud máxima; evitar esperas largas por una línea incompleta.

### Pruebas para darlo por terminado

1. Cada orden válida produce la acción y una respuesta coherente.
2. `/umbral 1.2`, `/umbral 9`, `/umbral abc`, `/umbral 1.2abc` y `/umbral` tienen resultados diferenciados y reproducibles.
3. Una orden bloqueada por modo automático no se registra en historial.
4. `/displayxyz` informa en ambos canales, sin desplazar el historial.
5. La serie y Telegram respetan las mismas reglas de actuación.
6. Perder Wi-Fi no detiene el control local y recuperar red permite volver a consultar mensajes.

## 8. Integrante 5: módulos Conectividad y Telemetria

### 8.1 Conectividad: responsabilidad e interfaz

Extraer `conectarWiFi()` y la gestión de reconexión. Este módulo es el único que llama a `WiFi.begin()` y administra el estado de enlace. No posee el cliente Telegram ni el cliente HTTP de ThingSpeak.

```cpp
namespace Conectividad {
  void iniciar(const char* ssid, const char* clave);
  void actualizar(unsigned long ahoraMs);
  bool estaConectado();
}
```

| Función | Parámetros | Resultado y comportamiento |
|---|---|---|
| `iniciar(ssid, clave)` | Credenciales con vigencia durante toda la aplicación | `void`. Inicia intento de conexión y retorna; no usa el bucle infinito actual. |
| `actualizar(ahoraMs)` | Tiempo actual | `void`. Detecta desconexión e inicia reintentos espaciados por configuración; no reinicia la conexión en cada vuelta. |
| `estaConectado()` | Ninguno | `true` cuando el enlace Wi-Fi está conectado; no garantiza que Internet o el servidor respondan. |

Diferenciar configuración para red física y para simulación sin editar el código de cada módulo. El `.ino` pasa las credenciales y realiza la actualización periódica.

### 8.2 Telemetria: responsabilidad e interfaz

Extraer `enviarDatosThingSpeak()`, su cliente HTTP y los temporizadores. Reactivar el envío periódico fuera del módulo Telegram. Para la consigna 01 se envían temperatura a `field1` y humedad a `field2`; el voltaje no reemplaza esos campos.

```cpp
namespace Telemetria {
  void iniciar(const char* apiKey, unsigned long ahoraMs);
  ResultadoEnvio enviarAhora(const Mediciones& mediciones,
                             unsigned long ahoraMs);
  ResultadoEnvio actualizar(const Mediciones& mediciones,
                            unsigned long ahoraMs);
}
```

| Función | Parámetros | Resultado y comportamiento |
|---|---|---|
| `iniciar(apiKey, ahoraMs)` | API key con vigencia permanente y tiempo inicial | `void`. Inicializa planificación y enfriamiento compartido. El primer envío periódico vence tras un intervalo; un envío manual puede intentarse antes si no hubo intentos previos. |
| `enviarAhora(mediciones, ahoraMs)` | Mediciones y tiempo | Intenta un envío manual y devuelve `ResultadoEnvio`. No omite el intervalo mínimo entre intentos HTTP. |
| `actualizar(mediciones, ahoraMs)` | Mediciones y tiempo | Devuelve `NoCorresponde` si aún no vence la evaluación periódica. Cuando vence, intenta mediante la misma rutina que el manual y devuelve el resultado real. |

### Contrato de resultados y temporizadores

- Comprobar primero Wi-Fi y validez DHT. Sin red: `SinWifi`. Con DHT inválido o valores no finitos: `DatosInvalidos`. No abrir HTTP en ninguno de esos casos.
- Si existe un intento HTTP demasiado reciente, devolver `IntervaloNoCumplido`, sin hacer otra solicitud. Manual y periódico comparten ese control.
- Para cada intento HTTP real, guardar la marca de finalización usando `millis()` después de cerrar la operación. Esto evita medir el intervalo desde un tiempo anterior a una llamada lenta.
- Mantener además la marca de evaluación periódica. Al vencer, actualizarla aunque falte red o fallen los sensores, para no reintentar desde cada vuelta del `loop()`.
- Cualquier intento HTTP manual también reprograma la evaluación periódica desde su finalización, para evitar un envío periódico inmediato después.
- Confirmar `Enviado` únicamente con respuesta HTTP exitosa y cuerpo que represente íntegramente un identificador positivo de registro. No alcanza con que `GET()` devuelva un número mayor que cero.
- Devolver `ErrorHttp` ante error de transporte o estado HTTP no exitoso. Con HTTP exitoso pero registro no aceptado o cuerpo inesperado: `RechazadoServidor`.
- Completar `codigoHttp` cuando se haya hecho una solicitud e `idRegistro` solo ante éxito.
- Cerrar el cliente en todos los caminos que abrieron una operación y limitar sus tiempos de espera.
- No enviar mensajes Telegram desde Telemetria: Comandos traduce el resultado del envío manual y Telegram lo transporta. El `.ino` puede registrar el resultado periódico por serie.

### Relación con otros módulos y con el `.ino`

Telemetria consulta `Conectividad::estaConectado()` y recibe mediciones; no lee sensores ni toca OLED o LEDs. El `.ino` inicia ambos módulos, actualiza Wi-Fi en cada vuelta y llama a Telemetria independientemente de que lleguen mensajes Telegram.

### Pruebas para darlo por terminado

1. Conectar, desconectar y recuperar Wi-Fi sin perder el control local.
2. Envío manual con registro confirmado en el canal correcto.
3. Envíos periódicos sin recibir comandos Telegram.
4. DHT inválido impide subir datos y devuelve una causa clara.
5. Dos `/thingup` seguidos respetan el intervalo; manual y periódico no envían juntos.
6. Respuesta HTTP fallida o cuerpo `0` nunca se informa como envío exitoso.

## 9. Trabajo del coordinador en el `.ino`

### Qué conserva el archivo principal

El `.ino` queda como punto de composición: inicia módulos, decide el orden de actualización y pasa instantáneas. Conserva `Serial.begin(115200)`, diagnósticos generales y el lector de comandos serie. No conserva objetos DHT, OLED, bot o HTTP ni variables duplicadas de sus estados.

### Esqueleto de integración propuesto

Este ejemplo muestra las llamadas acordadas. `procesarSerie` y `registrarResultadoPeriodico` son auxiliares del coordinador, no funciones de los módulos anteriores. El ejemplo no es una sustitución ya realizada del sketch.

```cpp
#include "Tipos.h"
#include "Config.h"
#include "Secretos.h"
#include "Sensores.h"
#include "Actuadores.h"
#include "Pantalla.h"
#include "Conectividad.h"
#include "Telemetria.h"
#include "Comandos.h"
#include "Telegram.h"

void procesarSerie(const Mediciones& mediciones, unsigned long ahoraMs);
void registrarResultadoPeriodico(const ResultadoEnvio& resultado);

void setup() {
  Serial.begin(115200);
  Sensores::iniciar();
  Actuadores::iniciar();
  if (!Pantalla::iniciar()) {
    Serial.println("OLED no disponible");
  }
  Conectividad::iniciar(WIFI_SSID, WIFI_PASSWORD);
  Telemetria::iniciar(THINGSPEAK_API_KEY, millis());
  Telegram::iniciar(BOT_TOKEN);
}

void loop() {
  Conectividad::actualizar(millis());
  Sensores::actualizar(millis());
  const Mediciones mediciones = Sensores::obtenerMediciones();

  Actuadores::actualizar(mediciones);
  procesarSerie(mediciones, millis());
  Telegram::actualizar(mediciones, millis());
  Actuadores::actualizar(mediciones);

  const ResultadoEnvio envio = Telemetria::actualizar(mediciones, millis());
  registrarResultadoPeriodico(envio);
  Pantalla::actualizar(mediciones, Actuadores::obtenerEstado(), millis());
}
```

Se actualiza el control antes y después de las órdenes para que sus cambios queden reflejados en la misma vuelta. Comandos también actualiza el control inmediatamente después de cambiar modo/umbral, porque puede haber varias órdenes dentro de una consulta Telegram.

`procesarSerie` acumula una línea completa, ejecuta `Comandos::ejecutar(linea, OrigenComando::Serie, mediciones, millis())` e imprime su respuesta. No vuelve a implementar las reglas de LEDs o umbral. `registrarResultadoPeriodico` ignora `NoCorresponde` para no saturar el monitor.

Capturar `millis()` cerca de cada llamada evita usar un tiempo viejo después de una operación de red. Aun así, la latencia de HTTP/Telegram limita la rapidez del lazo: comprobarla en la placa y ajustar tiempos de espera si perjudica el control.

### Mapa de dependencias permitido

```text
tpTA.ino -> Sensores, Actuadores, Pantalla, Conectividad,
            Telemetria, Telegram, Comandos
Telegram -> Comandos, Conectividad
Comandos -> Actuadores, Pantalla, Telemetria
Telemetria -> Conectividad
Sensores, Actuadores, Pantalla -> tipos/configuración común
```

No crear dependencias inversas: Pantalla no llama a Telegram y Telemetria no llama a Comandos. `Tipos.h` no incluye los módulos. Los `.h` públicos incluyen solo lo necesario; las dependencias concretas y bibliotecas externas se incluyen en sus `.cpp` cuando sea posible.

## 10. Secuencia de trabajo para evitar conflictos

1. **Contrato común:** el coordinador crea tipos, configuración y declaraciones públicas con los dueños. Todos parten del mismo commit. Si usa cuerpos temporales para compilar, deben quedar identificados como pendientes y nunca confundirse con funcionalidades terminadas.
2. **Ramas:** cada dueño crea su rama desde `main` actualizado: `sensores`, `actuadores`, `pantalla`, `telegram`, `conectividad-telemetria`.
3. **Primera integración:** sensores y actuadores. Probar lecturas y control usando serie.
4. **Segunda integración:** pantalla, conservando las operaciones anteriores.
5. **Tercera integración:** conectividad y telemetría. Probar periodicidad y resultado del envío manual.
6. **Cuarta integración:** comandos y Telegram. El dueño puede programarlos antes contra los contratos acordados; no hace falta esperar a que todos terminen para avanzar en su rama.
7. **Cierre conjunto:** simulación, placa UTN, documentación y ensayo de defensa.

El coordinador conecta un módulo dentro de la rama del pull request o en una rama de integración que incluya sus commits. Debe compilar y comprobar el caso principal antes de fusionar a `main`. No subir una versión intermedia que dejó llamadas sin implementación en la rama principal.

Para actualizar una rama de trabajo, primero guardar cambios propios en commits y luego ejecutar `git fetch origin` y `git merge origin/main`. Los conflictos se resuelven con el dueño del archivo, conservando el contrato acordado y compilando nuevamente.

Cada pull request indica:

- Archivos modificados y operaciones implementadas.
- Pruebas realizadas y evidencia breve de sus resultados.
- Ajustes que necesita el `.ino`, sin obligar a todos a editarlo.
- Cambios propuestos al contrato y consumidores afectados, si existen.

## 11. Lista de verificación de la entrega

- [ ] Confirmar que al grupo le corresponde la consigna 01.
- [ ] DHT22, GPIO y cableado coinciden con el enunciado y la placa.
- [ ] Cada objeto físico o de red y cada estado mutable tienen un único dueño.
- [ ] El sketch compila con todas las bibliotecas propias y externas necesarias.
- [ ] Están documentadas las versiones/dependencias que realmente se usaron para compilar.
- [ ] Todos los comandos previstos están en la ayuda y funcionan.
- [ ] Umbral inválido restaura 0.5 V y lo comunica.
- [ ] El modo automático utiliza línea base y umbral, con reglas iguales desde serie y Telegram.
- [ ] ThingSpeak recibe temperatura y humedad periódicamente y por `/thingup`.
- [ ] El historial conserva tres comandos Telegram exitosos sin superponer texto.
- [ ] `/display` desconocido informa en OLED y Telegram.
- [ ] Una falla de DHT, OLED o Wi-Fi tiene un comportamiento comprobado y no genera espera infinita deliberada.
- [ ] No quedan credenciales reales nuevas en archivos compartidos; se reemplazaron las que estaban expuestas.
- [ ] Se entrega el proyecto Wokwi, el `.ino` y todos los `.h`/`.cpp` necesarios, junto con instrucciones de configuración.
- [ ] Se ejecutó el conjunto sobre la placa UTN; la simulación por sí sola no cierra este punto.
- [ ] Cada integrante puede explicar su módulo, sus contratos y el recorrido completo del sistema.

Este documento especifica el trabajo pendiente. Sus casillas no indican pruebas ya realizadas ni certifican funcionamiento de la base actual.
