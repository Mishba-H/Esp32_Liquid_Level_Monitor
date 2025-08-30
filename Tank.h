#ifndef TANK_H
#define TANK_H

#include <Arduino.h>

/**
 * @class Tank
 * @brief Manages liquid tank parameters and calculates level, volume, and percentage.
 * * This class is designed for use with an ESP32 to monitor a liquid tank.
 * It loads its configuration from a JSON file stored in the ESP32's
 * filesystem (SPIFFS or LittleFS) and provides methods to calculate
 * the liquid's current depth, volume, and fill percentage based on sensor readings.
 */
class Tank {
public:
    /**
     * @brief Construct a new Tank object.
     * Initializes all parameters to zero.
     */
    Tank();

    /**
     * @brief Initializes the Tank object by loading parameters from a JSON file.
     * This method should be called in your setup() function. It assumes the
     * filesystem (e.g., SPIFFS) has already been initialized.
     * * @param filename The path to the JSON configuration file (e.g., "/tank_params.json").
     * @return true if initialization was successful, false otherwise.
     */
    bool begin(const char* filename = "/tank_params.json");

    /**
     * @brief Updates the current liquid level with a new reading from a sensor.
     * * @param distance The raw distance measurement from your sensor (e.g., ultrasonic)
     * to the surface of the liquid. The unit should be consistent
     * with the units used in the JSON config (e.g., meters or cm).
     */
    void updateLiquidLevel(float distance);

    /**
     * @brief Calculates the current depth of the liquid in the tank.
     * * @return The calculated liquid depth. Returns 0 if the reading is out of bounds.
     */
    float getLiquidDepth();

    /**
     * @brief Calculates the current volume of the liquid in the tank.
     * This calculation assumes a uniform cross-section (e.g., a cylindrical or
     * rectangular tank).
     * * @return The calculated liquid volume.
     */
    float getLiquidVolume();

    /**
     * @brief Calculates the fill percentage of the tank.
     * * @return The fill level as a percentage (0.0 to 100.0).
     */
    float getLiquidPercentage();

    /**
     * @brief Gets the full tank capacity.
     * @return The full tank capacity as defined in the config file.
     */
    float getFullCapacity() const;


private:
    // Configuration parameters loaded from JSON
    float _fullTankCapacity;       // Total volume of the tank when full (e.g., in Liters).
    float _tankCrossSectionArea;   // The area of the tank's base (e.g., in square meters).
    float _tankDepth;              // The total internal depth/height of the tank (e.g., in meters).
    float _sensorOffset;           // Distance from the top edge of the tank to the sensor's measurement point.

    // State variable
    float _currentSensorDistance;  // The latest raw distance reading from the sensor.
};

#endif // TANK_H
