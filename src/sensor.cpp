/*
Handle sensor setup and logging
*/

#include <core.h>

#include "Arduino.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <cstdint>
#include <list>

#include <ArduinoJson.h>

#include <Adafruit_Sensor.h>

#include "sensors/bme688.cpp"
#include "sensors/scd4x.cpp"
#include "sensors/tsl2591.cpp"
#include "sensors/sgp40.cpp"

#define SEALEVELPRESSURE_HPA (1013.25)

extern Config config;

// char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
uint32_t lastLogTimestamp = 0;

typedef std::list<BaseSensor*> BaseSensorList;
BaseSensorList sensorList;

BME688Sensor bme688_sensor;
SCD4xSensor scd4x_sensor;
TSL2591Sensor tsl2591_sensor;
SGP40Sensor sgp40_sensor;

const char *data_delimiter = "\n";

void mergeDocs(JsonObject dest, JsonObjectConst src)
{
    for (JsonPairConst kvp : src)
    {
        dest[kvp.key()] = kvp.value();
    }
};

void debugSensors(uint32_t timestamp)
{
    Serial.println(String("Debugging sensors: ") + timestamp);

    for (BaseSensorList::const_iterator iter = sensorList.begin(),
                                        endIter = sensorList.end();
         iter != endIter;
         ++iter)
    {
        BaseSensor *sensor = *iter;
        Serial.println(String("Running debug for sensor: ") + sensor->getName());
        sensor->debug();
    }
}

void logSensorDataToFile(uint32_t timestamp, String data_location)
{
    DynamicJsonDocument doc(600);
    doc["time_rtc"] = timestamp;

    for (BaseSensorList::const_iterator iter = sensorList.begin(),
                                      endIter = sensorList.end();
         iter != endIter;
         ++iter)
    {
        BaseSensor *sensor = *iter;
        DynamicJsonDocument sensor_doc = sensor->getData();
        mergeDocs(doc.as<JsonObject>(), sensor_doc.as<JsonObject>());

    }

    // DynamicJsonDocument bme688_doc = bme688_sensor.getData();
    // mergeDocs(doc.as<JsonObject>(), bme688_doc.as<JsonObject>());

    File file = SD.open(data_location, FILE_WRITE);
    if (!file)
    {
        Serial.println(F("Failed to open file"));
        return;
    }

    // Serialize JSON to file
    if (serializeJson(doc, file) == 0)
    {
        Serial.println(F("Failed to write to file"));
        return;
    }

    file.print(data_delimiter);

    // Close the file
    file.close();
}

bool setupSensors() {
    sensorList.push_back(&bme688_sensor);
    sensorList.push_back(&scd4x_sensor);
    sensorList.push_back(&tsl2591_sensor);
    sensorList.push_back(&sgp40_sensor);

    // return bme68_sensor.setup();
    for (BaseSensorList::const_iterator iter = sensorList.begin(),
                                        endIter = sensorList.end();
         iter != endIter;
         ++iter)
    {
        BaseSensor *sensor = *iter;
        Serial.println(String("Setting up sensor: ") + sensor->getName());
        if (sensor->setup() == false) {
            Serial.println(String("Sensor setup failed: ") + sensor->getName());
        }
    }

    return true;
};

void loopSensors(uint32_t timestamp, String data_location)
{
    uint32_t timeLogDiff = timestamp - lastLogTimestamp;
    if (timeLogDiff > config.logFreq)
    {
        logSensorDataToFile(timestamp, data_location);
        lastLogTimestamp = timestamp;
    }
}