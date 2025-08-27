#include <Esp32WebServer.h>

Esp32WebServer webServer;

void setup() {
    Serial.begin(115200);
    webServer.begin();
}

void loop() {
    webServer.update();
}
