#ifndef CustomESP32UDP_h
#define CustomESP32UDP_h

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUDP.h>

class CustomESP32UDP {
public:
  CustomESP32UDP();
  void begin(const char* ssid, const char* password, const char* host, int remotePort, int localPort);
  void sendAnalogData(int* pins, int numPins);
  void update(int delayMillis);
private:
  WiFiUDP udp;
  char ssid_[32];
  char password_[64];
  char host_[16];
  int remotePort_;
  int localPort_;
  int* pins_;
  int numPins_;
  unsigned long lastSendTime;
  int customDelay;
};

#endif
