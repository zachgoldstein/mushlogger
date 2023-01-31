#include <sensors/base_sensor.h>
#include "Arduino.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <cstdint>

#include "SparkFun_SCD4x_Arduino_Library.h"

#include <ArduinoJson.h>

class SCD4xSensor: public BaseSensor {
    private:
        SCD4x co2_sensor;
        String name = "SCD40 Co2 Sensor";

    public:
    bool setup()
    {
        Serial.println("Setting up SCD4x CO2 sensor");
        co2_sensor.enableDebugging();

        //.begin will start periodic measurements for us (see the later examples for details on how to override this)
        if (co2_sensor.begin() == false)
        {
            Serial.println(F("CO2 Sensor not detected"));
            return false;
        }
        Serial.println("Finished setting up SCD4x CO2 sensor");

        return true;
    }

    void debug() {
        Serial.println("Debug SCD4x Sensor");

        if (co2_sensor.readMeasurement()) // readMeasurement will return true when fresh data is available
        {
            Serial.println();
            Serial.print(F("SCD4x Sensor..."));

            Serial.print(F("CO2(ppm):"));
            Serial.print(co2_sensor.getCO2());

            Serial.print(F("\tTemperature(C):"));
            Serial.print(co2_sensor.getTemperature(), 1);

            Serial.print(F("\tHumidity(%RH):"));
            Serial.print(co2_sensor.getHumidity(), 1);

            Serial.println();
        }
    }
    
    void loop(){
        
    }

    String getName() {
        return name;
    }

    DynamicJsonDocument getData()
    {
        // Note: The SCD4x CO2 sensor has data ready every five seconds
        // Measure CO2 with SCD4x sensor
        co2_sensor.readMeasurement();

        DynamicJsonDocument doc(200);
        doc["CO2_SCD40"] = co2_sensor.getCO2();
        doc["temperature_SCD40"] = co2_sensor.getTemperature();
        doc["humidity_SCD40"] = co2_sensor.getHumidity();

        return doc;
    }
};

