#include <sensors/base_sensor.h>
#include "Arduino.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <cstdint>

#include <Adafruit_AHTX0.h>
#include <ArduinoJson.h>

class AHT20Sensor: public BaseSensor {
    private:
        Adafruit_AHTX0 aht;

        const char *name = "AHT20 Temp/Humidity Sensor";

    public:
        bool setup()
        {
            Serial.println("Setting up the AHT humidity/temp/pressure sensor");
            if (!aht.begin())
            {
                Serial.println("Could not find AHT? Check wiring");
                return false;
            }
            Serial.println("Finished up the AHT humidity/temp/pressure sensor");

            return true;
        }

        void debug() {
            Serial.println("Debug AHT20 Sensor");

            sensors_event_t humidity, temp;
            aht.getEvent(&humidity, &temp); // populate temp and humidity objects with fresh data
            Serial.print("Temperature: ");
            Serial.print(temp.temperature);
            Serial.println(" degrees C");
            Serial.print("Humidity: ");
            Serial.print(humidity.relative_humidity);
            Serial.println("% rH");
            Serial.println();
        }
        
        void loop(){
            
        }

        char getName() {
            return *name;
        }

        DynamicJsonDocument getData()
        {
            // Measure humidty and temp with AHT20 sensor
            sensors_event_t humidity, temp;
            aht.getEvent(&humidity, &temp); // populate temp and humidity objects with fresh data

            DynamicJsonDocument doc(200);
            doc["temperature_aht20"] = temp.temperature;
            doc["humidity_aht20"] = humidity.relative_humidity;
            return doc;
        }
};

