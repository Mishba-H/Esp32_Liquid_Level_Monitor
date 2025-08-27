#ifndef Esp32WebServer_h
#define Esp32WebServer_h

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
    String _ssid;
    String _password;
    AsyncWebServer _server;
    AsyncWebSocket _ws;
    unsigned long _updateTimeMS;
    int _valueA;
    String _valueB;

    static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                          AwsEventType type, void *arg, uint8_t *data, size_t len);

    void loadConfig();
    void saveConfig();
};

#endif