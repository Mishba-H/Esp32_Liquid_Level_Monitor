#include <Esp32WebServer.h>
#include <Scheduler.h>

constexpr unsigned long ultrasonicSensorDelay = 1000;

Esp32WebServer webServer;

void UltrasonicSensorTask()
{
    Serial.println("test");
}

void setup() {
    Serial.begin(115200);
    webServer.begin();
}

void loop() {
    Scheduler::repeat<ultrasonicSensorDelay, UltrasonicSensorTask>();
    webServer.update();
}
