#include "CustomESP32UDP.h"

CustomESP32UDP::CustomESP32UDP() {
  // Constructor
  numVariables_ = 0;
  data_ = nullptr;
  signal_ = 0;
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

void CustomESP32UDP::sendData(int* data, int numVariables) {
  data_ = data;
  numVariables_ = numVariables;
}

void CustomESP32UDP::sendSignal(int signal) {
  signal_ = signal;
}

void CustomESP32UDP::update(int delayMillis) {
  if (WiFi.status() == WL_CONNECTED && millis() - lastSendTime > delayMillis) {
    String data = "d ";
    for (int i = 0; i < numVariables_; i++) {
      data += String(data_[i]) + " ";
    }
    
    udp.beginPacket(host_, remotePort_);
    udp.print(data);
    udp.endPacket();
    lastSendTime = millis();
  }
}

void CustomESP32UDP::updateSignal(int delayMillis) {
  if (WiFi.status() == WL_CONNECTED && millis() - lastSendTime > delayMillis) {
    String signal = "s " + String(signal_);
    
    udp.beginPacket(host_, remotePort_);
    udp.print(signal);
    udp.endPacket();
    lastSendTime = millis();
  }
}