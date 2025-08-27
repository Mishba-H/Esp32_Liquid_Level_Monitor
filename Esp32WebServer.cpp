#include "Esp32WebServer.h"

Esp32WebServer::Esp32WebServer()
    : _server(80), _ws("/ws"), _updateTimeMS(200), _valueA(0), _valueB("default") {}

void Esp32WebServer::loadConfig() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return;
    }

    if (!SPIFFS.exists("/config.json")) {
        Serial.println("Config file not found, writing default...");
        saveConfig(); // create default config
        return;
    }

    File file = SPIFFS.open("/config.json", "r");
    if (!file) {
        Serial.println("Failed to open config.json");
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.print("Failed to parse config.json: ");
        Serial.println(error.c_str());
        return;
    }

    _ssid         = doc["wifi_ssid"]      | "defaultSSID";
    _password     = doc["wifi_password"]  | "defaultPASS";
    _updateTimeMS = doc["serverUpdateTime"] | 200;
    _valueA       = doc["valueA"]         | 0;
    _valueB       = doc["valueB"]         | "default";

    Serial.println("Config loaded:");
    serializeJsonPretty(doc, Serial);
    Serial.println();
}

void Esp32WebServer::saveConfig() {
    StaticJsonDocument<512> doc;
    doc["wifi_ssid"]       = _ssid;
    doc["wifi_password"]   = _password;
    doc["serverUpdateTime"] = _updateTimeMS;
    doc["valueA"]          = _valueA;
    doc["valueB"]          = _valueB;

    File file = SPIFFS.open("/config.json", "w");
    if (!file) {
        Serial.println("Failed to open config.json for writing");
        return;
    }

    if (serializeJsonPretty(doc, file) == 0) {
        Serial.println("Failed to write config.json");
    }
    file.close();
    Serial.println("Config saved.");
}

void Esp32WebServer::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                               AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client #%u connected from %s\n",
                      client->id(), client->remoteIP().toString().c_str());
    } 
    else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    }
    else if (type == WS_EVT_DATA) {
        Serial.printf("WebSocket data received (%u bytes)\n", (unsigned)len);
        // Example: handle JSON updates from client
    }
}

void Esp32WebServer::begin() {
    loadConfig();

    Serial.printf("Connecting to SSID: %s\n", _ssid.c_str());
    WiFi.begin(_ssid.c_str(), _password.c_str());

    Serial.println("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Connected..!");
    Serial.print("Got IP: ");
    Serial.println(WiFi.localIP());

    _ws.onEvent(onWsEvent);
    _server.addHandler(&_ws);

    _server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html", "text/html");
    });

    _server.begin();
}

void Esp32WebServer::update() {
    _ws.cleanupClients();
}
