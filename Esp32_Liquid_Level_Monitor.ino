#include <Esp32WebServer.h>
#include <Scheduler.h>
#include <Arduino.h>

#pragma region WEB_SERVER
Esp32WebServer webServer;

void startWebServer()
{
    webServer.begin();
}
#pragma endregion

#pragma region LOAD_PIN_CONFIG
int triggerPin;
int echoPin;
int pushbuttonPin;

void setPinMode(int pin, const char* modeStr) {
  if (strcmp(modeStr, "OUTPUT") == 0) {
    pinMode(pin, OUTPUT);
  } else if (strcmp(modeStr, "INPUT") == 0) {
    pinMode(pin, INPUT);
  } else if (strcmp(modeStr, "INPUT_PULLUP") == 0) {
    pinMode(pin, INPUT_PULLUP);
  } else {
    Serial.printf("Unknown mode '%s' for pin %d\n", modeStr, pin);
  }
}

void loadPinConfig() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  File file = SPIFFS.open("/pin_config.json", "r");
  if (!file) {
    Serial.println("Failed to open pin_config.json");
    return;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.f_str());
    return;
  }

  for (JsonPair kv : doc.as<JsonObject>()) {
    const char* name = kv.key().c_str(); 
    int number = kv.value()["number"];      
    const char* mode = kv.value()["mode"];    

    if (strcmp(name, "triggerPin") == 0) triggerPin = number;
    else if (strcmp(name, "echoPin") == 0) echoPin = number;
    else if (strcmp(name, "pushbuttonPin") == 0) pushbuttonPin = number;

    setPinMode(number, mode);

    Serial.printf("Configured %s = %d as %s\n", name, number, mode);
  }
}
#pragma endregion

#pragma region LOAD_TASK_DELAY_CONFIG
unsigned long pushButtonTaskDelay = 0;
unsigned long ultrasonicSensorTaskDelay = 1000;

void loadTaskDelayConfig() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  File file = SPIFFS.open("/task_delay_config.json", "r");
  if (!file) {
    Serial.println("Failed to open task_delay_config.json");
    return;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.f_str());
    return;
  }

  pushButtonTaskDelay   = doc["pushButtonTaskDelay"] | pushButtonTaskDelay;
  ultrasonicSensorTaskDelay = doc["ultrasonicSensorTaskDelay"] | ultrasonicSensorTaskDelay;

  Serial.println("Task delay configuration loaded:");
}
#pragma endregion

#pragma region PUSH_BUTTON_TASK
int currentVal;
int previousVal = LOW; 
unsigned long lastPushTime = 0UL;           // the last time the output pin was toggled
unsigned long debounce = 200UL;   // the debounce time, increase if the output flickers

void pushButtonTask() {
    currentVal = digitalRead(pushbuttonPin);

    if (currentVal == HIGH && previousVal == LOW && millis() - lastPushTime > debounce)
    {
        // push detected
        Serial.println("push button pushed");
    }

    previousVal = currentVal;
}
#pragma endregion

#pragma region ULTRASONIC_SENSOR_TASK
void ultrasonicSensorTask() {
    Serial.println("test");
}
#pragma endregion

void setup() {
    Serial.begin(115200);

    loadPinConfig();
    loadTaskDelayConfig();

    startWebServer();
}

void loop() {
    Scheduler::repeat<pushButtonTask>(pushButtonTaskDelay);
    Scheduler::repeat<ultrasonicSensorTask>(ultrasonicSensorTaskDelay);
    webServer.update();
}
