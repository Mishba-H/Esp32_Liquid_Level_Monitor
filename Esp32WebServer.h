#ifndef ESP32_WEB_SERVER_H
#define ESP32_WEB_SERVER_H

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

enum WifiMode {
  MODE_AP,
  MODE_STA
};

class Esp32WebServer {
public:
  Esp32WebServer();
  void begin();
  void update();
  void toggleWifiMode();
  WifiMode getCurrentMode();
  void sendTankData(const String &data);

private:
  AsyncWebServer _server;
  AsyncWebSocket _ws;
  WifiMode _currentMode;
  String _ap_ssid, _ap_password, _sta_ssid, _sta_password;

  void startAPMode();
  void startSTAMode();
  void configureServerRoutes();
  void loadNetworkConfig(bool setModeFromConfig);

  static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
  static void handleJsonSave(AsyncWebServerRequest *request, const String &filename, uint8_t *data, size_t len, size_t index, size_t total);
  static void handleTankSave(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
};

#endif