#include "Esp32WebServer.h"

Esp32WebServer::Esp32WebServer()
    : _server(80), _ws("/ws"), _currentMode(MODE_AP) {}

WifiMode Esp32WebServer::getCurrentMode() {
    return _currentMode;
}

void Esp32WebServer::begin() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS. Halting.");
        while(1) delay(100);
    }
    
    loadNetworkConfig(true); 

    configureServerRoutes();

    if (_currentMode == MODE_AP) {
        startAPMode();
    } else {
        startSTAMode();
    }
}

void Esp32WebServer::loadNetworkConfig(bool setModeFromConfig) {
    File file = SPIFFS.open("/config/network_config.json", "r");
    if (!file) {
        Serial.println("Failed to open network_config.json. Defaulting to AP mode.");
        _currentMode = MODE_AP;
        _ap_ssid = "ESP32-Fallback-AP";
        _ap_password = "12345678";
        return;
    }
    StaticJsonDocument<512> doc;
    deserializeJson(doc, file);
    file.close();

    if (setModeFromConfig) {
        String mode = doc["default_mode"] | "AP";
        _currentMode = (mode == "STA") ? MODE_STA : MODE_AP;
    }

    // Always load the credentials
    _ap_ssid = doc["wifi_ap_ssid"].as<String>();
    _ap_password = doc["wifi_ap_password"].as<String>();
    _sta_ssid = doc["wifi_sta_ssid"].as<String>();
    _sta_password = doc["wifi_sta_password"].as<String>();
    
    Serial.println("Network config loaded/reloaded.");
}

void Esp32WebServer::toggleWifiMode() {
    Serial.println("\n--- TOGGLING WIFI MODE ---");
    _server.end();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);

    _currentMode = (_currentMode == MODE_AP) ? MODE_STA : MODE_AP;

    if (_currentMode == MODE_AP) {
        startAPMode();
    } else {
        startSTAMode();
    }
}

void Esp32WebServer::startAPMode() {
    loadNetworkConfig(false); 

    Serial.printf("Starting AP mode. SSID: %s\n", _ap_ssid.c_str());
    WiFi.softAP(_ap_ssid.c_str(), _ap_password.c_str());
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    _server.begin();
    Serial.println("Server started in AP Mode.");
}

void Esp32WebServer::startSTAMode() {
    loadNetworkConfig(false); 
    
    Serial.printf("Starting STA mode. Connecting to SSID: %s\n", _sta_ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(_sta_ssid.c_str(), _sta_password.c_str());

    Serial.print("Connecting to WiFi...");
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 30) {
        delay(500); Serial.print("."); retries++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nFailed to connect to STA WiFi. Reverting to AP mode.");
        toggleWifiMode(); return;
    }
    
    Serial.println("\nConnected..!");
    Serial.print("IP Address: "); Serial.println(WiFi.localIP());
    _server.begin();
    Serial.println("Server started in STA Mode.");
}

// --- The rest of the file is the same ---
void Esp32WebServer::configureServerRoutes() {
    _ws.onEvent(onWsEvent);
    _server.addHandler(&_ws);
    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (this->_currentMode == MODE_AP) {
            request->send(SPIFFS, "/website_config/index.html", "text/html");
        } else {
            request->send(SPIFFS, "/website_monitor/index.html", "text/html");
        }
    });
    _server.on("/style.css", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (this->_currentMode == MODE_AP) {
            request->send(SPIFFS, "/website_config/style.css", "text/css");
        } else {
            request->send(SPIFFS, "/website_monitor/style.css", "text/css");
        }
    });
    _server.on("/script.js", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (this->_currentMode == MODE_AP) {
            request->send(SPIFFS, "/website_config/script.js", "application/javascript");
        } else {
            request->send(SPIFFS, "/website_monitor/script.js", "application/javascript");
        }
    });
    _server.on("/api/network", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(SPIFFS, "/config/network_config.json", "application/json"); });
    _server.on("/api/pins", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(SPIFFS, "/config/pin_config.json", "application/json"); });
    _server.on("/api/system", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(SPIFFS, "/config/system_config.json", "application/json"); });
    _server.on("/api/tank", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(SPIFFS, "/config/tank_params.json", "application/json"); });
    _server.on("/api/save/network", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t l, size_t i, size_t t){ handleJsonSave(request, "/config/network_config.json", data, l, i, t); });
    _server.on("/api/save/pins", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t l, size_t i, size_t t){ handleJsonSave(request, "/config/pin_config.json", data, l, i, t); });
    _server.on("/api/save/system", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t l, size_t i, size_t t){ handleJsonSave(request, "/config/system_config.json", data, l, i, t); });
    _server.on("/api/save/tank", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t l, size_t i, size_t t){ handleJsonSave(request, "/config/tank_params.json", data, l, i, t); });
    _server.onNotFound([](AsyncWebServerRequest *request){ request->send(404, "text/plain", "Not found"); });
}
void Esp32WebServer::handleJsonSave(AsyncWebServerRequest *request, const String& filename, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index + len != total) return;
    File file = SPIFFS.open(filename, "w");
    if (!file) { request->send(500, "text/plain", "SERVER ERROR: Could not open config file."); return; }
    if (file.write(data, total) == total) { request->send(200, "text/plain", "OK"); } 
    else { request->send(500, "text/plain", "SERVER ERROR: File write failed."); }
    file.close();
}
void Esp32WebServer::sendTankData(const String& data) {
    _ws.textAll(data);
}
void Esp32WebServer::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) { Serial.printf("WS client #%u connected\n", client->id()); } 
    else if (type == WS_EVT_DISCONNECT) { Serial.printf("WS client #%u disconnected\n", client->id()); }
}
void Esp32WebServer::update() {
    _ws.cleanupClients();
}