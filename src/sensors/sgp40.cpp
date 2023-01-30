#include <sensors/base_sensor.h>
#include "Arduino.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <cstdint>

#include "Adafruit_SGP40.h"
#include <ArduinoJson.h>

class SGP40Sensor: public BaseSensor {
    private:
        Adafruit_SGP40 sgp40;
        const char *name = "SGP40 VOC Index sensor";

    public:
    bool setup()
    {
        Serial.println("Setting up the SGP40 Gas Sensor");
        if (!sgp40.begin()){
            Serial.println("SGP40 sensor not found :(");
            return false;
        }
        Serial.print("Found SGP40 serial #");
        Serial.print(sgp40.serialnumber[0], HEX);
        Serial.print(sgp40.serialnumber[1], HEX);
        Serial.println(sgp40.serialnumber[2], HEX);

        return true;
    }

    void debug() {
        Serial.println("Debug SGP40 Sensor");
        Serial.println();
        Serial.print(F("SGP40 Sensor..."));

        Serial.print("SGP raw gas measurement");
        Serial.print(sgp40.measureRaw());
        Serial.print("SGP VOC index");
        Serial.print(sgp40.measureVocIndex());
    }
    
    void loop(){
        
    }

    char getName() {
        return *name;
    }

    DynamicJsonDocument getData()
    {
        sgp40.measureRaw();
        sgp40.measureVocIndex();

        DynamicJsonDocument doc(200);
        doc["raw_gas_sgp40"] = sgp40.measureRaw();
        doc["VOC_Index_sgp40"] = sgp40.measureVocIndex();
        return doc;
    }
};

