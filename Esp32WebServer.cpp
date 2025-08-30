#include "Esp32WebServer.h"

Esp32WebServer::Esp32WebServer()
    : _server(80), _ws("/ws"), _currentMode(MODE_AP) {}

void Esp32WebServer::begin() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS. Halting.");
        while(1) delay(100);
    }
    
    configureServerRoutes(); // Configure routes ONCE
    
    loadNetworkConfig(); 

    if (_currentMode == MODE_AP) {
        startAPMode();
    } else {
        startSTAMode();
    }
}

void Esp32WebServer::loadNetworkConfig() {
    File file = SPIFFS.open("/config/network_config.json", "r");
    if (!file) {
        Serial.println("Failed to open network_config.json. Defaulting to AP mode.");
        _currentMode = MODE_AP;
        _ap_ssid = "ESP32-Fallback-AP";
        _ap_password = "12345678";
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("Failed to parse network_config.json. Using previously loaded values.");
        return;
    }

    String mode = doc["default_mode"] | "AP";
    _currentMode = (mode == "STA") ? MODE_STA : MODE_AP;
    _ap_ssid = doc["wifi_ap_ssid"].as<String>();
    _ap_password = doc["wifi_ap_password"].as<String>();
    _sta_ssid = doc["wifi_sta_ssid"].as<String>();
    _sta_password = doc["wifi_sta_password"].as<String>();
    
    Serial.println("Network config loaded/reloaded.");
}

void Esp32WebServer::saveNetworkMode() {
    File file = SPIFFS.open("/config/network_config.json", "r");
    StaticJsonDocument<512> doc;
    if (file) { deserializeJson(doc, file); file.close(); }

    doc["default_mode"] = (_currentMode == MODE_AP) ? "AP" : "STA";

    file = SPIFFS.open("/config/network_config.json", "w");
    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write updated mode to network_config.json");
    } else {
        Serial.println("Saved new default mode to config file.");
    }
    file.close();
}

void Esp32WebServer::toggleWifiMode() {
    Serial.println("\n--- TOGGLING WIFI MODE ---");
    _server.end(); // Stop server before changing WiFi
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);

    _currentMode = (_currentMode == MODE_AP) ? MODE_STA : MODE_AP;
    saveNetworkMode();

    if (_currentMode == MODE_AP) {
        startAPMode();
    } else {
        startSTAMode();
    }
}

void Esp32WebServer::startAPMode() {
    loadNetworkConfig();
    Serial.printf("Starting AP mode. SSID: %s\n", _ap_ssid.c_str());
    WiFi.softAP(_ap_ssid.c_str(), _ap_password.c_str());
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    _server.begin();
    Serial.println("Server started in AP Mode.");
}

void Esp32WebServer::startSTAMode() {
    loadNetworkConfig();
    Serial.printf("Starting STA mode. Connecting to SSID: %s\n", _sta_ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(_sta_ssid.c_str(), _sta_password.c_str());

    Serial.print("Connecting to WiFi...");
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 30) {
        delay(500);
        Serial.print(".");
        retries++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nFailed to connect to STA WiFi. Reverting to AP mode.");
        toggleWifiMode();
        return;
    }
    
    Serial.println("\nConnected..!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    _server.begin();
    Serial.println("Server started in STA Mode.");
}

void Esp32WebServer::configureServerRoutes() {
    _ws.onEvent(onWsEvent);
    _server.addHandler(&_ws);

    // **THE FIX**: This single handler now serves the correct page based on the current mode.
    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (this->_currentMode == MODE_AP) {
            request->send(SPIFFS, "/website_config/index.html", "text/html");
        } else { // MODE_STA
            request->send(200, "text/html", "<h1>ESP32 is in Station Mode</h1><p>Press the physical button to return to Configuration Mode (AP).</p>");
        }
    });

    // Serve static files for the config website
    _server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(SPIFFS, "/website_config/style.css", "text/css"); });
    _server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(SPIFFS, "/website_config/script.js", "application/javascript"); });

    // API endpoints for GETTING configs
    _server.on("/api/network", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(SPIFFS, "/config/network_config.json", "application/json"); });
    _server.on("/api/pins", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(SPIFFS, "/config/pin_config.json", "application/json"); });
    _server.on("/api/tasks", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(SPIFFS, "/config/task_delay_config.json", "application/json"); });

    // API endpoints for SAVING configs
    _server.on("/api/save/network", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){ handleJsonSave(request, "/config/network_config.json", data, len, index, total); });
    _server.on("/api/save/pins", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){ handleJsonSave(request, "/config/pin_config.json", data, len, index, total); });
    _server.on("/api/save/tasks", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){ handleJsonSave(request, "/config/task_delay_config.json", data, len, index, total); });

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

void Esp32WebServer::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) { Serial.printf("WS client #%u connected\n", client->id()); } 
    else if (type == WS_EVT_DISCONNECT) { Serial.printf("WS client #%u disconnected\n", client->id()); }
}

void Esp32WebServer::update() {
    _ws.cleanupClients();
}