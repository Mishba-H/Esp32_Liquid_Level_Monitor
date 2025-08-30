#ifndef ESP32_WEB_SERVER_H
#define ESP32_WEB_SERVER_H

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

enum WifiMode { MODE_AP, MODE_STA };

class Esp32WebServer {
public:
    Esp32WebServer();
    void begin();
    void update();
    void toggleWifiMode();

private:
    AsyncWebServer _server;
    AsyncWebSocket _ws;

    // WiFi Credentials and Mode
    WifiMode _currentMode;
    String _ap_ssid;
    String _ap_password;
    String _sta_ssid;
    String _sta_password;
    
    // Server setup helpers
    void startAPMode();
    void startSTAMode();
    void configureServerRoutes();

    // Config management
    void loadNetworkConfig();
    void saveNetworkMode();

    // WebSocket event handler
    static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    
    // Generic handler to save a JSON file from a POST request
    static void handleJsonSave(AsyncWebServerRequest *request, const String& filename, uint8_t *data, size_t len, size_t index, size_t total);
};

#endif