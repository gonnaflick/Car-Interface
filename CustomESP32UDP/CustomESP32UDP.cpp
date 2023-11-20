#include "CustomESP32UDP.h"

CustomESP32UDP::CustomESP32UDP() {
  // Constructor
}

void CustomESP32UDP::begin(const char* ssid, const char* password, const char* host, int remotePort, int localPort) {
  strcpy(ssid_, ssid);
  strcpy(password_, password);
  strcpy(host_, host);
  remotePort_ = remotePort;
  localPort_ = localPort;
  lastSendTime = 0;

  WiFi.begin(ssid_, password_);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a la red Wi-Fi...");
  }

  Serial.println("Conexión Wi-Fi establecida.");
  udp.begin(localPort_);
  Serial.println("Comunicación UDP iniciada.");
}

void CustomESP32UDP::sendAnalogData(int* pins, int numPins) {
  pins_ = pins;
  numPins_ = numPins;
}

void CustomESP32UDP::update(int delayMillis) {
  if (WiFi.status() == WL_CONNECTED && millis() - lastSendTime > delayMillis) {
    String data = "";
    for (int i = 0; i < numPins_; i++) {
      int value = analogRead(pins_[i]);
      data += "Pin " + String(pins_[i]) + ": " + String(value) + ", ";
    }
    
    udp.beginPacket(host_, remotePort_);
    udp.print(data);
    udp.endPacket();
    lastSendTime = millis();
    Serial.println("Datos enviados por UDP.");
  }
}
