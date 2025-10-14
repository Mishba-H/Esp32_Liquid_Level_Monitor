#pragma once

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "Tank.h"

class Esp32WebServer {
public:
  Esp32WebServer(Tank* tank, unsigned long* measurementInterval, unsigned long* responseTimeout);
  void begin();
  void enable();
  void disable();
  void cleanupClients();

private:
  // Pointers to global system objects/variables
  Tank* sys_tank;
  unsigned long* sys_measurementInterval;
  unsigned long* sys_responseTimeout;

  bool m_enabled;
  AsyncWebServer m_server;
  AsyncWebSocket m_ws;
  String m_ssid;
  String m_password;

  void loadNetworkConfig();
  void configureServerRoutes();

  // Helper to read a config file into a JsonDocument
  bool readConfigFile(const char* filename, JsonDocument& doc);

  // Static function for WebSocket events
  static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
};