#include "Tank.h"
#include <ArduinoJson.h>  // Make sure you have the ArduinoJson library installed
#include <FS.h>           // Filesystem header
#include <SPIFFS.h>       // Using SPIFFS as the example filesystem

// Define which filesystem to use. Change to LittleFS if you prefer.
#define FileSystem SPIFFS

Tank::Tank()
  : _fullTankCapacity(0.0f),
    _tankCrossSectionArea(0.0f),
    _tankDepth(0.0f),
    _sensorOffset(0.0f),
    _currentSensorDistance(0.0f) {
}

bool Tank::begin(const char* filename) {
  // Note: It's assumed that the filesystem (e.g., SPIFFS.begin()) has been
  // initialized before this method is called. We'll add a check just in case.
  if (!FileSystem.begin(true)) {
    Serial.println("An Error has occurred while mounting the filesystem");
    return false;
  }

  File configFile = FileSystem.open(filename, "r");
  if (!configFile) {
    Serial.print("Failed to open config file: ");
    Serial.println(filename);
    return false;
  }

  // Allocate a JSON document.
  // The size should be adjusted based on the complexity of your file.
  // The ArduinoJson Assistant can help calculate this: https://arduinojson.org/v6/assistant/
  StaticJsonDocument<256> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, configFile);
  if (error) {
    Serial.print("Failed to parse config file, error: ");
    Serial.println(error.c_str());
    configFile.close();
    return false;
  }

  // Extract values from the JSON document
  _fullTankCapacity = doc["full_tank_capacity"] | 0.0f;
  _tankCrossSectionArea = doc["tank_cross_section_area"] | 0.0f;
  _tankDepth = doc["tank_depth"] | 0.0f;
  _sensorOffset = doc["sensor_offset"] | 0.0f;  // Added sensor offset parameter

  configFile.close();

  // Basic validation
  if (_fullTankCapacity <= 0 || _tankCrossSectionArea <= 0 || _tankDepth <= 0) {
    Serial.println("Invalid tank parameters loaded from JSON. Values must be positive.");
    return false;
  }

  Serial.println("Tank parameters loaded successfully:");
  Serial.print("- Full Capacity: ");
  Serial.println(_fullTankCapacity);
  Serial.print("- Cross-Section Area: ");
  Serial.println(_tankCrossSectionArea);
  Serial.print("- Tank Depth: ");
  Serial.println(_tankDepth);
  Serial.print("- Sensor Offset: ");
  Serial.println(_sensorOffset);

  return true;
}

void Tank::updateLiquidLevel(float distance) {
  _currentSensorDistance = distance;
}

float Tank::getLiquidDepth() {
  if (_tankDepth <= 0) return 0;

  // Calculate the actual depth of the liquid
  float liquidDepth = _tankDepth - (_currentSensorDistance - _sensorOffset);

  // Constrain the value between 0 and the total tank depth
  if (liquidDepth < 0) {
    liquidDepth = 0;
  } else if (liquidDepth > _tankDepth) {
    liquidDepth = _tankDepth;
  }

  return liquidDepth;
}

float Tank::getLiquidVolume() {
  if (_tankCrossSectionArea <= 0) return 0;

  // Calculate volume based on the liquid depth and cross-section area.
  // This assumes a uniform shape (cylinder, rectangle, etc.).
  // Note: The units must be consistent. If depth is in meters and area is in
  // square meters, the volume will be in cubic meters.
  float volume = getLiquidDepth() * _tankCrossSectionArea;

  // You might want to convert this to a more common unit, like Liters.
  // Example: 1 cubic meter = 1000 Liters. If your config uses meters,
  // you would multiply the result by 1000 to get Liters.
  // For this implementation, we return the direct calculation.

  return volume;
}

float Tank::getLiquidPercentage() {
  if (_tankDepth <= 0) return 0;

  float percentage = (getLiquidDepth() / _tankDepth) * 100.0f;

  // Clamp the value between 0 and 100
  if (percentage < 0) {
    percentage = 0;
  } else if (percentage > 100) {
    percentage = 100;
  }

  return percentage;
}

float Tank::getFullCapacity() const {
  return _fullTankCapacity;
}
