#include <WiFi.h>
#include <CustomESP32UDP.h>

#define CLK 25
#define DT 26
#define SW 27

#define DIRECTION_CW 0   // clockwise direction
#define DIRECTION_CCW 1  // counter-clockwise direction

const char* ssid = "INFINITUM63D9_2.4";
const char* password = "0936263930";
const char* host = "192.168.1.14";
int remotePort = 15000;
int localPort = 8888;

WiFiServer server(23); // Puerto TCP para el servidor

int numData = 7;
int dataBuffer[7];

// Definir los pines de salida para los focos LED
const int pinFocoIzquierda = 4; // Pin para el foco de la izquierda
const int pinFocoDerecha = 5;   // Pin para el foco de la derecha
const int pinLucesCortas = 15;   // Pin para las luces cortas
const int pinLucesLargas = 21;    // Pin para las luces largas
const int pinCinturon = 22;      // Pin para cinturon
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

int counter = 0;
int currentStateCLK;
int lastStateCLK;
String currentDIR = "";
unsigned long lastButtonPress = 0;

CustomESP32UDP esp32UDP;

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
  pinMode(pinCinturon, INPUT_PULLUP);

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

  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);
  lastStateCLK = digitalRead(CLK);
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
  int btnState = digitalRead(SW);

  currentStateCLK = digitalRead(CLK);

  if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {

    if (digitalRead(DT) == currentStateCLK) {
      if (counter >= -50)
        counter--;
    }
    else {
      if (counter <= 50)
        counter++;
    }

    Serial.println(counter);
  }

  lastStateCLK = currentStateCLK;


  if (btnState == LOW) {
    if (millis() - lastButtonPress > 50) {
      Serial.println("Fish meter");
    }

    lastButtonPress = millis();
  }
  // Leer el estado de los botones
  bool aceleradorPresionado = digitalRead(pinAcelerador) == LOW;
  bool frenoPresionado = digitalRead(pinFreno) == LOW;

  if (counter <= -33 && velocidad != 0 && estadoIntermitenteIzquierda == true && estadoIntermitenteDerecha != true || counter >= 33 && velocidad != 0 && estadoIntermitenteDerecha == true && estadoIntermitenteIzquierda != true) {
    esp32UDP.sendSignal(1);
    esp32UDP.updateSignal(1);
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
  dataBuffer[4] = !digitalRead(pinCinturon);
  dataBuffer[5] = velocidad;
  dataBuffer[6] = counter;

  // Envía los datos de lectura por UDP
  esp32UDP.sendData(dataBuffer, numData);

  esp32UDP.update(1);

  delay(1);
}
