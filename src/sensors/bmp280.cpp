#include <sensors/base_sensor.h>
#include "Arduino.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <cstdint>

#include <Adafruit_BMP280.h>
#include <ArduinoJson.h>

class XXXSensor: public BaseSensor {
    private:
        Adafruit_BMP280 bmp; // I2C

        const char *name = "BMP280 Temp/Humidity/Pressure Sensor";

    public:
        bool setup()
        {
            Serial.println("Setting up BPM280 pressure/temp sensor");
            unsigned status;
            status = bmp.begin();
            if (!status)
            {
                Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                                "try a different address!"));
                Serial.print("SensorID was: 0x");
                Serial.println(bmp.sensorID(), 16);
                Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
                Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
                Serial.print("        ID of 0x60 represents a BME 280.\n");
                Serial.print("        ID of 0x61 represents a BME 680.\n");
                return false;
            }

            /* Default settings from datasheet. */
            bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                            Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                            Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                            Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                            Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
            Serial.println("Finished setting up BPM280 sensor");
            return true;
        }

        void debug() {
            Serial.println();
            Serial.println("Debug BMP280 Sensor");

            Serial.print(F("Temperature = "));
            Serial.print(bmp.readTemperature());
            Serial.println(" *C");

            Serial.print(F("Pressure = "));
            Serial.print(bmp.readPressure() / 1000.0);
            Serial.println(" Pa");

            Serial.print(F("Approx altitude = "));
            Serial.print(bmp.readAltitude(1013.25)); /* Adjusted to local forecast! */
            Serial.println(" m");

            Serial.println();
        }
        
        void loop(){
            
        }

        char getName() {
            return *name;
        }

        DynamicJsonDocument getData()
        {

            DynamicJsonDocument doc(200);
            doc["altitude_bmp280"] = bmp.readAltitude(1013.25);
            doc["pressure_bmp280"] = bmp.readPressure() / 100.0;
            doc["temperature_bmp280"] = bmp.readTemperature();
            return doc;
        }
};

