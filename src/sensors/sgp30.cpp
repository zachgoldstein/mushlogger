#include <sensors/base_sensor.h>
#include "Arduino.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <cstdint>

#include "Adafruit_SGP30.h"
#include <ArduinoJson.h>

class XXXSensor: public BaseSensor {
    private:
        Adafruit_SGP30 sgp30;
        const char *name = "SGP30 eCO2 Sensor";
        uint32_t lastsgp30BaselineLogTimestamp = 0;
        uint32_t sgp30BaselineLogFreq = 30; // Log sgp30 baseline every 30 seconds;

        /* return absolute humidity [mg/m^3] with approximation formula
         * @param temperature [°C]
         * @param humidity [%RH]
         */
        uint32_t getAbsoluteHumidity(float temperature, float humidity)
        {
            // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
            const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
            const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity);                                                                // [mg/m^3]
            return absoluteHumidityScaled;
        }

    public:
        bool setup()
        {
            Serial.println("Setting up the SGP30 eCO2 sensor");
            if (!sgp30.begin())
            {
                Serial.println("SGP30 eCO2 Sensor not found :(");
                return false;
            }
            Serial.print("Found SGP30 serial #");
            Serial.print(sgp30.serialnumber[0], HEX);
            Serial.print(sgp30.serialnumber[1], HEX);
            Serial.println(sgp30.serialnumber[2], HEX);

            // If you have a baseline measurement from before you can assign it to start, to 'self-calibrate'
            sgp30.setIAQBaseline(0x8E68, 0x8F41); // Will vary for each sensor!
            Serial.println("Finished setting up the SGP30 eCO2 sensor");

            return true;
        }

        void debug() {
            Serial.println("Debug SGP30 Sensor");
            // sgp30.setHumidity(getAbsoluteHumidity(temp.temperature, humidity.relative_humidity));
            if (!sgp30.IAQmeasure())
            {
                Serial.println("SGP Measurement failed");
                return;
            }
            Serial.println();
            Serial.print(F("SGP30 Sensor..."));

            Serial.print("SGP TVOC ");
            Serial.print(sgp30.TVOC);
            Serial.print(" ppb\t");
            Serial.print("SGP eCO2 ");
            Serial.print(sgp30.eCO2);
            Serial.println(" ppm");
        }
        
        void loop(){
            // uint32_t timeSGP30LogDiff = timestamp - lastsgp30BaselineLogTimestamp;
            // if (timeSGP30LogDiff > sgp30BaselineLogFreq)
            // {
            //     lastsgp30BaselineLogTimestamp = timestamp;
            //     uint16_t TVOC_base, eCO2_base;
            //     if (!sgp.getIAQBaseline(&eCO2_base, &TVOC_base))
            //     {
            //         Serial.println("Failed to get baseline readings");
            //         return;
            //     }
            //     Serial.print("****Baseline values: eCO2: 0x");
            //     Serial.print(eCO2_base, HEX);
            //     Serial.print(" & TVOC: 0x");
            //     Serial.println(TVOC_base, HEX);
            // }
        }

        char getName() {
            return *name;
        }
        
        void configureHumidity(float temp, float humidity) {
            sgp30.setHumidity(getAbsoluteHumidity(temp, humidity));
        }

        DynamicJsonDocument getData()
        {
            // Measure eCO2 and VOC with SGP30 sensor
            if (!sgp30.IAQmeasure())
            {
                Serial.println("SGP IAQmeasure failed");
            }

            DynamicJsonDocument doc(200);
            // doc["temperature_bme688"] = bme688.temperature;
            // doc["pressure_bme688"] = bme688.pressure / 100.0;
            // doc["humidity_bme688"] = bme688.humidity;
            // doc["gas_resistance_bme688"] = bme688.gas_resistance / 1000.0;
            return doc;
        }
};
