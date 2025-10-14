#include "Esp32WebServer.h"

// Constructor: Initializes the server and websocket, and stores pointers to system variables.
Esp32WebServer::Esp32WebServer(Tank* tank, unsigned long* measurementInterval, unsigned long* responseTimeout)
  : m_server(80), m_ws("/ws") {
  m_enabled = false;
  sys_tank = tank;
  sys_measurementInterval = measurementInterval;
  sys_responseTimeout = responseTimeout;
}

// Loads initial network config and sets up WebSocket events. Called once in setup().
void Esp32WebServer::begin() {
  loadNetworkConfig();
  m_ws.onEvent(onWsEvent);
}

// Enables the WiFi Access Point and starts the web server.
void Esp32WebServer::enable() {
  if (m_enabled) return;

  WiFi.softAP(m_ssid.c_str(), m_password.c_str());
  Serial.println("Access Point Enabled.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  configureServerRoutes();
  m_server.addHandler(&m_ws);
  m_server.begin();
  m_enabled = true;
}

// Disables the web server and WiFi Access Point.
void Esp32WebServer::disable() {
  if (!m_enabled) return;
  m_server.end();
  m_ws.closeAll();
  WiFi.softAPdisconnect(true);
  Serial.println("Access Point & Web Server Disabled.");
  m_enabled = false;
}

// Cleans up any disconnected WebSocket clients.
void Esp32WebServer::cleanupClients() {
  if (m_enabled) {
    m_ws.cleanupClients();
  }
}

// Loads the WiFi credentials from SPIFFS.
void Esp32WebServer::loadNetworkConfig() {
  JsonDocument doc;
  if (readConfigFile("/config/network_params.json", doc)) {
      m_ssid = doc["ssid"] | "Liquid_Level_Monitor";
      m_password = doc["password"] | "12345678";
      Serial.println("Network config loaded.");
  } else {
      Serial.println("network_params.json not found or failed to parse. Using defaults.");
      m_ssid = "Liquid_Level_Monitor";
      m_password = "12345678";
  }
}

// Helper function to read and parse a JSON file from SPIFFS.
bool Esp32WebServer::readConfigFile(const char* filename, JsonDocument& doc) {
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        return false;
    }
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    return !error;
}


// --- Server Route Configuration ---

void Esp32WebServer::configureServerRoutes() {
  // Serve main webpage and assets
  m_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });
  m_server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });
  m_server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/script.js", "application/javascript");
  });

  // API: GET all configurations
  m_server.on("/api/get-configs", HTTP_GET, [this](AsyncWebServerRequest *request) {
    JsonDocument combinedDoc;
    JsonDocument tempDoc;

    if (readConfigFile("/config/tank_params.json", tempDoc)) {
        combinedDoc["tankCrossSectionAreaCm2"] = tempDoc["tankCrossSectionAreaCm2"];
        combinedDoc["tankDepthCm"] = tempDoc["tankDepthCm"];
        combinedDoc["sensorOffsetCm"] = tempDoc["sensorOffsetCm"];
        combinedDoc["heightErrorMarginCm"] = tempDoc["heightErrorMarginCm"];
    }
    if (readConfigFile("/config/system_params.json", tempDoc)) {
        combinedDoc["measurementInterval"] = tempDoc["measurementInterval"];
        combinedDoc["responseTimeout"] = tempDoc["responseTimeout"];
    }
    if (readConfigFile("/config/network_params.json", tempDoc)) {
        combinedDoc["ssid"] = tempDoc["ssid"];
        combinedDoc["password"] = tempDoc["password"];
    }

    String output;
    serializeJson(combinedDoc, output);
    request->send(200, "application/json", output);
  });

  // API: POST to update tank configuration
  m_server.on("/api/update-tank", HTTP_POST, [](AsyncWebServerRequest * request){}, NULL,
    [this](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      if (deserializeJson(doc, (char*)data) == DeserializationError::Ok) {
          File file = SPIFFS.open("/config/tank_params.json", "w");
          serializeJson(doc, file);
          file.close();
          sys_tank->loadConfig(); // Reload tank config immediately
          request->send(200, "application/json", "{\"status\":\"success\", \"message\":\"Tank settings saved!\"}");
      } else {
          request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Invalid JSON\"}");
      }
  });

  // API: POST to update system configuration
  m_server.on("/api/update-system", HTTP_POST, [](AsyncWebServerRequest * request){}, NULL,
    [this](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      if (deserializeJson(doc, (char*)data) == DeserializationError::Ok) {
          File file = SPIFFS.open("/config/system_params.json", "w");
          serializeJson(doc, file);
          file.close();
          // Update global variables via pointers
          *sys_measurementInterval = doc["measurementInterval"];
          *sys_responseTimeout = doc["responseTimeout"];
          request->send(200, "application/json", "{\"status\":\"success\", \"message\":\"System settings applied!\"}");
      } else {
          request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Invalid JSON\"}");
      }
  });

  // API: POST to update network configuration
  m_server.on("/api/update-network", HTTP_POST, [](AsyncWebServerRequest * request){}, NULL,
    [this](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      if (deserializeJson(doc, (char*)data) == DeserializationError::Ok) {
          File file = SPIFFS.open("/config/network_params.json", "w");
          serializeJson(doc, file);
          file.close();
          // Update local variables. Restart is needed for AP to change.
          m_ssid = doc["ssid"].as<String>();
          m_password = doc["password"].as<String>();
          request->send(200, "application/json", "{\"status\":\"success\", \"message\":\"Network settings saved. Restart device to apply.\"}");
      } else {
          request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Invalid JSON\"}");
      }
  });

  // Handle Not Found
  m_server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });
}


// --- WebSocket Event Handler ---

void Esp32WebServer::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    // FIX: Changed %u to %lu for uint32_t client ID
    Serial.printf("WS client #%lu connected from %s\n", client->id(), client->remoteIP().toString().c_str());
  } else if (type == WS_EVT_DISCONNECT) {
    // FIX: Changed %u to %lu for uint32_t client ID
    Serial.printf("WS client #%lu disconnected\n", client->id());
  } else if (type == WS_EVT_DATA) {
    // Handle data from client if needed in the future
  }
}