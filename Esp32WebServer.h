#ifndef ESP32_WEB_SERVER_H
#define ESP32_WEB_SERVER_H

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

class Esp32WebServer {
public:
    Esp32WebServer();
    void begin();
    void update();

private:
    AsyncWebServer _server;
    AsyncWebSocket _ws;
    String _ssid;
    String _password;
    unsigned long _updateTimeMS;

    static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    void loadNetworkConfig();
    void saveNetworkConfig();
};

#endif