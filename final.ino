#include <WiFi.h>
#include <CustomESP32UDP.h>
#include <ezButton.h>  // the library to use for SW pin

#define CLK_PIN 25  // ESP32 pin GPIO25 connected to the rotary encoder's CLK pin
#define DT_PIN 26   // ESP32 pin GPIO26 connected to the rotary encoder's DT pin
#define SW_PIN 27   // ESP32 pin GPIO27 connected to the rotary encoder's SW pin

#define DIRECTION_CW 0   // clockwise direction
#define DIRECTION_CCW 1  // counter-clockwise direction

const char* ssid = "INFINITUM63D9_2.4";
const char* password = "0936263930";
const char* host = "192.168.1.14";
int remotePort = 15000;
int localPort = 8888;

WiFiServer server(23); // Puerto TCP para el servidor

int dataBuffer[6]; // Tamaño para acomodar las lecturas de 4 pines
int numData = sizeof(dataBuffer) / sizeof(dataBuffer[0]);

// Definir los pines de salida para los focos LED
const int pinFocoIzquierda = 4; // Pin para el foco de la izquierda
const int pinFocoDerecha = 5;   // Pin para el foco de la derecha
const int pinLucesCortas = 15;   // Pin para las luces cortas
const int pinLucesLargas = 21;    // Pin para las luces largas
const int pinLucesFreno = 23;    // Pin para las luces freno
const int pinAcelerador = 18; // Asumiendo D18 como acelerador
const int pinFreno = 19;      // Asumiendo D19 como freno

// Variables para el control de intermitencia
bool estadoIntermitenteIzquierda = false;
bool estadoIntermitenteDerecha = false;
unsigned long previousMillisIzquierda = 0; // Para manejar la intermitencia de la luz izquierda
unsigned long previousMillisDerecha = 0;   // Para manejar la intermitencia de la luz derecha
const long interval = 500; // Intervalo de la intermitencia en milisegundos

float velocidad = 0.0; // Velocidad actual del "automóvil"
float aceleracion = 0.001; // Cantidad de aumento de velocidad por ciclo
float desaceleracion = 0.005; // Cantidad de disminución de velocidad por ciclo
float frenado = 0.05; // Cantidad de reducción de velocidad al frenar
unsigned long tiempoAnterior = 0; // Para rastrear el tiempo desde el último ciclo
unsigned long tiempoPresionadoAcelerador = 0; // Tiempo que el acelerador ha estado presionado

volatile int counter = 0;
volatile int direction = DIRECTION_CW;
volatile unsigned long last_time;  // for debouncing
int prev_counter;

ezButton button(SW_PIN);  // create ezButton object that attach to pin 7;

CustomESP32UDP esp32UDP;

void IRAM_ATTR ISR_encoder() {
  if ((millis() - last_time) < 10)  // debounce time is 50ms
    return;

  if (digitalRead(DT_PIN) == HIGH && counter > -50) {
    // the encoder is rotating in counter-clockwise direction => decrease the counter
    counter--;
    direction = DIRECTION_CCW;
  } else {
    // the encoder is rotating in clockwise direction => increase the counter
    if(counter < 50) {
      counter++;
      direction = DIRECTION_CW;
    }
  }

  last_time = millis();
}

void sendClientMessage(const String& message) {
  WiFiClient tempClient = server.available();
  if (tempClient) {
    tempClient.println(message);
    tempClient.stop();
  }
}

void setup() {
  Serial.begin(115200);
  esp32UDP.begin(ssid, password, host, remotePort, localPort);
  
  // Conectar a la red WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // Configurar los pines como salidas y los pines de entrada
  pinMode(pinFocoIzquierda, OUTPUT);
  pinMode(pinFocoDerecha, OUTPUT);
  pinMode(pinLucesCortas, OUTPUT);
  pinMode(pinLucesLargas, OUTPUT);
  pinMode(pinLucesFreno, OUTPUT);

  pinMode(pinAcelerador, INPUT_PULLUP);
  pinMode(pinFreno, INPUT_PULLUP);

  // Inicializar los focos LED y el pinDefaultOn encendido
  digitalWrite(pinFocoIzquierda, LOW);
  digitalWrite(pinFocoDerecha, LOW);
  digitalWrite(pinLucesCortas, LOW);
  digitalWrite(pinLucesLargas, LOW);
  digitalWrite(pinLucesFreno, HIGH);

  // Iniciar el servidor
  server.begin();
  
  // Imprimir la dirección IP local
  Serial.println("");
  Serial.println("WiFi conectado.");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // configure encoder pins as inputs
  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  button.setDebounceTime(50);  // set debounce time to 50 milliseconds

  // use interrupt for CLK pin is enough
  // call ISR_encoder() when CLK pin changes from LOW to HIGH
  attachInterrupt(digitalPinToInterrupt(CLK_PIN), ISR_encoder, RISING);
}

// Función para controlar las luces cortas y largas
void controlarLucesCortasLargas(String command) {
  if (command == "cortas") {
    digitalWrite(pinLucesCortas, HIGH);
    digitalWrite(pinLucesLargas, LOW);
  } else if (command == "largas") {
    digitalWrite(pinLucesCortas, HIGH);
    digitalWrite(pinLucesLargas, HIGH);
  } else if (command == "off") {
    digitalWrite(pinLucesCortas, LOW);
    digitalWrite(pinLucesLargas, LOW);
  }
}

// Función para alternar la luz basada en la intermitencia
void toggleLight(bool &estadoIntermitente, unsigned long &previousMillis, const int pinFoco) {
  if(estadoIntermitente && millis() - previousMillis >= interval) {
    bool estadoFoco = digitalRead(pinFoco);
    digitalWrite(pinFoco, !estadoFoco);
    previousMillis = millis();
  }
}

void loop() {
  button.loop();  // MUST call the loop() function first
  // Leer el estado de los botones
  bool aceleradorPresionado = digitalRead(pinAcelerador) == LOW;
  bool frenoPresionado = digitalRead(pinFreno) == LOW;

  if (counter <= -33 && velocidad != 0 && estadoIntermitenteIzquierda == true && estadoIntermitenteDerecha != true || counter >= 33 && velocidad != 0 && estadoIntermitenteDerecha == true && estadoIntermitenteIzquierda != true) {
    Serial.println("intermitente_izquierda_desactivada");
    sendClientMessage("apagar");
  }

  // Escuchar clientes entrantes
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Nuevo cliente conectado");
    while (client.connected()) {
      if (client.available()) {
        // Leer el mensaje del cliente
        String command = client.readStringUntil('\n');
        command.trim(); // Eliminar espacios en blanco y caracteres de nueva línea

        // Procesar el comando recibido y ajustar los estados de intermitencia
        if (command == "izquierda") {
            estadoIntermitenteIzquierda = true;
          estadoIntermitenteDerecha = false;
          digitalWrite(pinFocoDerecha, LOW); // Apagar el foco derecho si estaba encendido
        } else if (command == "derecha") {
            estadoIntermitenteDerecha = true;
          estadoIntermitenteIzquierda = false;
          digitalWrite(pinFocoIzquierda, LOW); // Apagar el foco izquierdo si estaba encendido
        } else if (command == "ambas") {
          estadoIntermitenteIzquierda = true;
          estadoIntermitenteDerecha = true;
        } else if (command == "apagar") {
          estadoIntermitenteIzquierda = false;
          estadoIntermitenteDerecha = false;
          digitalWrite(pinFocoIzquierda, LOW);
          digitalWrite(pinFocoDerecha, LOW);
        }
        
        // Controlar luces cortas y largas
        controlarLucesCortasLargas(command);
        
        // Enviar respuesta al cliente
        client.println("Comando recibido: " + command);
      }
    }
    // Cerrar la conexión con el cliente
    client.stop();
    Serial.println("Cliente desconectado");
  }

  // Manejar la intermitencia de cada luz
  toggleLight(estadoIntermitenteIzquierda, previousMillisIzquierda, pinFocoIzquierda);
  toggleLight(estadoIntermitenteDerecha, previousMillisDerecha, pinFocoDerecha);unsigned long tiempoActual = millis();


  // Acelerar
  if (aceleradorPresionado) {
    tiempoPresionadoAcelerador += tiempoActual - tiempoAnterior;
    velocidad += aceleracion * (tiempoPresionadoAcelerador / 1000.0); // Aumentar la velocidad exponencialmente
    digitalWrite(pinLucesFreno, LOW); // Apagar luces de frenado
  }

  // Frenar
  if (frenoPresionado) {
    velocidad -= frenado; // Reducir la velocidad casi instantáneamente
    tiempoPresionadoAcelerador = 0; // Restablecer el tiempo de aceleración
    digitalWrite(pinLucesFreno, HIGH); // Encender luces de frenado
  }

  // Desaceleración natural exponencial
if (!aceleradorPresionado && !frenoPresionado) {
  if (velocidad > 0) {
    velocidad -= pow(1 / velocidad, 0.05) * desaceleracion;
  }
  if (velocidad < 1) {
    digitalWrite(pinLucesFreno, HIGH); // Encender luces de frenado
  }
  tiempoPresionadoAcelerador = 0; // Restablecer el tiempo de aceleración
}

  // Prevenir que la velocidad sea negativa
  if (velocidad < 0) {
    velocidad = 0;
    digitalWrite(pinLucesFreno, HIGH); // Encender luces de frenado
  }

  // Actualizar el tiempo anterior
  tiempoAnterior = tiempoActual;

  // Realiza lecturas de los pines y almacena los valores en dataBuffer
  dataBuffer[0] = digitalRead(pinFocoIzquierda);
  dataBuffer[1] = digitalRead(pinFocoDerecha);
  dataBuffer[2] = digitalRead(pinLucesCortas);
  dataBuffer[3] = digitalRead(pinLucesLargas);
  dataBuffer[4] = velocidad;
  dataBuffer[5] = counter;

  // Actualiza numData para reflejar la cantidad de datos válidos
  numData = 6;

  // Envía los datos de lectura por UDP
  esp32UDP.sendData(dataBuffer, 6);

  esp32UDP.update(1);
}
