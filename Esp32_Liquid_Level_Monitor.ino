#include <Arduino.h>
#include "Esp32WebServer.h"
#include "Tank.h"

// --- Global Objects ---
Esp32WebServer webServer;
Tank tank;

// --- Pin Config Variables (loaded from JSON) ---
int triggerPin;
int echoPin;
int toggleWifiModeButtonPin;

// --- System Config Variables (loaded from JSON) ---
unsigned long toggleWifiModeInterval = 500; // Debounce
unsigned long measurementInterval = 1000; // How often to measure

// --- Button Handling ---
int currentButtonVal, previousButtonVal = LOW;
unsigned long lastPushTime = 0UL;

// --- Sensor State ---
unsigned long lastMeasurementMillis = 0;

// --- Forward Declarations ---
void loadPinConfig();
void loadSystemConfig();

void setup() {
    Serial.begin(115200);

    webServer.begin();

    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS. Halting.");
        while(1) delay(1);
    }

    loadPinConfig();
    loadSystemConfig();
    tank.begin("/config/tank_params.json");
}

void loop() {
    // --- Handle Button Press for WiFi Mode Toggle ---
    currentButtonVal = digitalRead(toggleWifiModeButtonPin);
    if (previousButtonVal == HIGH && currentButtonVal == LOW && millis() - lastPushTime > toggleWifiModeInterval) {
        lastPushTime = millis();
        Serial.println("Wifi Mode Toggle Button Pressed!");
        webServer.toggleWifiMode();
    }
    previousButtonVal = currentButtonVal;

    // --- In STA mode, perform measurements and send data ---
    if (webServer.getCurrentMode() == MODE_STA) {
        if (millis() - lastMeasurementMillis >= measurementInterval) {
            lastMeasurementMillis = millis();

            // 1. Perform measurement
            digitalWrite(triggerPin, LOW);
            delayMicroseconds(2);
            digitalWrite(triggerPin, HIGH);
            delayMicroseconds(10);
            digitalWrite(triggerPin, LOW);
            long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout

            if (duration > 0) {
                float distance_cm = duration * 0.0343 / 2.0;
                Serial.println(distance_cm);
                
                // 2. Update Tank object
                tank.updateLiquidLevel(distance_cm);

                // 3. Prepare JSON data
                StaticJsonDocument<256> doc;
                doc["depth"] = tank.getLiquidDepth();
                doc["volume"] = tank.getLiquidVolume();
                doc["percentage"] = tank.getLiquidPercentage();
                
                String jsonData;
                serializeJson(doc, jsonData);
                
                // 4. Broadcast via WebSocket
                webServer.sendTankData(jsonData);
            }
        }
    }
    webServer.update(); // Cleanup websocket clients
}

// --- Configuration Loading Functions ---
void loadPinConfig() {
    File file = SPIFFS.open("/config/pin_config.json", "r");
    if (!file) { Serial.println("Failed to open pin_config.json"); return; }
    StaticJsonDocument<512> doc;
    deserializeJson(doc, file);
    file.close();

    triggerPin = doc["triggerPin"]["number"];
    echoPin = doc["echoPin"]["number"];
    toggleWifiModeButtonPin = doc["toggleWifiModeButtonPin"]["number"];

    setPinMode(triggerPin, doc["triggerPin"]["mode"]);
    setPinMode(echoPin, doc["echoPin"]["mode"]);
    setPinMode(toggleWifiModeButtonPin, doc["toggleWifiModeButtonPin"]["mode"]);
    Serial.println("Pin config loaded.");
}

void loadSystemConfig() {
    File file = SPIFFS.open("/config/system_config.json", "r");
    if (!file) { Serial.println("Failed to open system_config.json"); return; }
    StaticJsonDocument<512> doc;
    deserializeJson(doc, file);
    file.close();

    toggleWifiModeInterval = doc["toggleWifiModeInterval"] | 200;
    measurementInterval = doc["measurementInterval"] | 1000;
    Serial.println("System config loaded.");
}

void setPinMode(int pin, const char *modeStr) {
    if (strcmp(modeStr, "OUTPUT") == 0) { pinMode(pin, OUTPUT); }
    else if (strcmp(modeStr, "INPUT") == 0) { pinMode(pin, INPUT); }
    else if (strcmp(modeStr, "INPUT_PULLUP") == 0) { pinMode(pin, INPUT_PULLUP); }
}