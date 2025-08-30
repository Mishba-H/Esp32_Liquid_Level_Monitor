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

void setPinMode(int pin, const char *modeStr)
{
  if (strcmp(modeStr, "OUTPUT") == 0)
  {
    pinMode(pin, OUTPUT);
  }
  else if (strcmp(modeStr, "INPUT") == 0)
  {
    pinMode(pin, INPUT);
  }
  else if (strcmp(modeStr, "INPUT_PULLUP") == 0)
  {
    pinMode(pin, INPUT_PULLUP);
  }
  else
  {
    Serial.printf("Unknown mode '%s' for pin %d\n", modeStr, pin);
  }
}

void loadPinConfig()
{
  File file = SPIFFS.open("/config/pin_config.json", "r");
  if (!file)
  {
    Serial.println("Failed to open pin_config.json");
    return;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error)
  {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.f_str());
    return;
  }

  for (JsonPair kv : doc.as<JsonObject>())
  {
    const char *name = kv.key().c_str();
    int number = kv.value()["number"];
    const char *mode = kv.value()["mode"];

    if (strcmp(name, "triggerPin") == 0)
      triggerPin = number;
    else if (strcmp(name, "echoPin") == 0)
      echoPin = number;
    else if (strcmp(name, "pushbuttonPin") == 0)
      pushbuttonPin = number;

    setPinMode(number, mode);

    Serial.printf("Configured %s = %d as %s\n", name, number, mode);
  }
}
#pragma endregion

#pragma region LOAD_TASK_DELAY_CONFIG
unsigned long pushButtonTaskDelay = 0;
unsigned long ultrasonicSensorTaskDelay = 1000;

void loadTaskDelayConfig()
{
  File file = SPIFFS.open("/config/task_delay_config.json", "r");
  if (!file)
  {
    Serial.println("Failed to open task_delay_config.json");
    return;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error)
  {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.f_str());
    return;
  }

  pushButtonTaskDelay = doc["pushButtonTaskDelay"] | pushButtonTaskDelay;
  ultrasonicSensorTaskDelay = doc["ultrasonicSensorTaskDelay"] | ultrasonicSensorTaskDelay;

  Serial.println("Task delay configuration loaded:");
}
#pragma endregion

#pragma region PUSH_BUTTON_TASK
volatile bool modeToggleRequested = false;
int currentVal;
int previousVal = LOW;
unsigned long lastPushTime = 0UL; // the last time the output pin was toggled
unsigned long debounce = 200UL;  // the debounce time, increase if the output flickers

void pushButtonTask() {
  currentVal = digitalRead(pushbuttonPin);

  // Detect falling edge: HIGH -> LOW
  if (previousVal == HIGH && currentVal == LOW) {
    if (millis() - lastPushTime > debounce) {
      lastPushTime = millis();
      Serial.println("push button pushed");
      modeToggleRequested = true;
    }
  }

  previousVal = currentVal;
}
#pragma endregion

#pragma region ULTRASONIC_SENSOR_TASK
enum UltrasonicState
{
  IDLE,
  SEND_PULSE,
  WAIT_FOR_ECHO_START,
  WAIT_FOR_ECHO_END
};

UltrasonicState ultrasonic_state = IDLE;
unsigned long ultrasonic_last_action_time = 0; // Time in microseconds
unsigned long ultrasonic_echo_start_time = 0;  // Time in microseconds

void ultrasonicSensorTask()
{
  // Only start a new measurement if the previous one is finished (i.e., state is IDLE)
  if (ultrasonic_state == IDLE)
  {
    digitalWrite(triggerPin, HIGH);
    ultrasonic_last_action_time = micros();
    ultrasonic_state = SEND_PULSE; // Move to the next state
  }
}

void handleUltrasonicSensor()
{
  // If a measurement is not in progress, do nothing.
  if (ultrasonic_state == IDLE)
  {
    return;
  }

  unsigned long current_micros = micros();

  switch (ultrasonic_state)
  {
  case SEND_PULSE:
    // End the trigger pulse after 10 microseconds.
    if (current_micros - ultrasonic_last_action_time >= 10)
    {
      digitalWrite(triggerPin, LOW);
      ultrasonic_state = WAIT_FOR_ECHO_START;
      // Start the timeout counter for the echo
      ultrasonic_last_action_time = current_micros;
    }
    break;

  case WAIT_FOR_ECHO_START:
    // Wait for the echo pulse to begin (echoPin goes HIGH).
    // A 38000 microsecond timeout is used in case no echo is received.
    if (current_micros - ultrasonic_last_action_time > 38000)
    {
      Serial.println("Distance: Echo Timeout!");
      ultrasonic_state = IDLE; // Reset
    }
    else if (digitalRead(echoPin) == HIGH)
    {
      ultrasonic_echo_start_time = current_micros;
      ultrasonic_state = WAIT_FOR_ECHO_END;
    }
    break;

  case WAIT_FOR_ECHO_END:
    // Wait for the echo pulse to end (echoPin goes LOW).
    if (digitalRead(echoPin) == LOW)
    {
      unsigned long duration = current_micros - ultrasonic_echo_start_time;
      float distance = (duration * 0.0343) / 2.0;

      Serial.print("Distance: ");
      Serial.println(distance);
      ultrasonic_state = IDLE;
    }
    break;
  }
}
#pragma endregion

void setup()
{
  Serial.begin(115200);

  if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return;
    }

  loadPinConfig();
  loadTaskDelayConfig();

  Scheduler::addTask<pushButtonTask>(pushButtonTaskDelay);
  Scheduler::addTask<ultrasonicSensorTask>(ultrasonicSensorTaskDelay);

  startWebServer();
  
  delay(1);
}

void loop()
{
  if (modeToggleRequested) 
  {
    webServer.toggleWifiMode();
    modeToggleRequested = false; 
  }
  webServer.update();
  Scheduler::update();

  handleUltrasonicSensor();
}
