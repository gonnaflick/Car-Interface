#ifndef CustomESP32UDP_h
#define CustomESP32UDP_h

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

class CustomESP32UDP {
public:
  CustomESP32UDP();
  void begin(const char* ssid, const char* password, const char* host, int remotePort, int localPort);
  void sendData(int* data, int numVariables);
  void sendSignal(int signal);
  void update(int delayMillis);
  void updateSignal(int delayMillis);

private:
  char ssid_[32];
  char password_[32];
  char host_[32];
  int remotePort_;
  int localPort_;
  WiFiUDP udp;
  int signal_;
  int* data_;
  int numVariables_;
  unsigned long lastSendTime;
};

#endif
