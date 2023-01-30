#include <sensors/base_sensor.h>
#include "Arduino.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <cstdint>

#include "Adafruit_BME680.h"
#include <ArduinoJson.h>

#define SEALEVELPRESSURE_HPA (1013.25)

class BME688Sensor: public BaseSensor {
    private:
    Adafruit_BME680 bme688;
    const char *name = "BME688 Temp/Pressure/Humidity Sensor";

    public:
    bool setup()
    {
        Serial.println("Setting up BPM688 sensor");
        // if (!bme688.begin(0x76)) for testing board
        if (!bme688.begin(0x77))
        {
            Serial.println(F("Could not find a valid BME680 sensor, check wiring!"));
            return false;
        }
        // Set up oversampling and filter initialization
        bme688.setTemperatureOversampling(BME680_OS_8X);
        bme688.setHumidityOversampling(BME680_OS_2X);
        bme688.setPressureOversampling(BME680_OS_4X);
        bme688.setIIRFilterSize(BME680_FILTER_SIZE_3);
        bme688.setGasHeater(320, 150); // 320*C for 150 ms
        Serial.println("Finished setting up BPM6880 sensor");
        return true;
    }

    void debug() {
        // Tell BME680 to begin measurement.
        unsigned long endTime = bme688.beginReading();
        if (endTime == 0)
        {
            Serial.println(F("Failed to begin reading :("));
            return;
        }
        Serial.print(F("Reading started at "));
        Serial.print(millis());
        Serial.print(F(" and will finish at "));
        Serial.println(endTime);

        Serial.println(F("You can do other work during BME680 measurement."));
        delay(50); // This represents parallel work.
        // There's no need to delay() until millis() >= endTime: bme.endReading()
        // takes care of that. It's okay for parallel work to take longer than
        // BME680's measurement time.

        // Obtain measurement results from BME680. Note that this operation isn't
        // instantaneous even if milli() >= endTime due to I2C/SPI latency.
        if (!bme688.endReading())
        {
            Serial.println(F("Failed to complete reading :("));
            return;
        }
        Serial.print(F("Reading completed at "));
        Serial.println(millis());

        Serial.print(F("Temperature = "));
        Serial.print(bme688.temperature);
        Serial.println(F(" *C"));

        Serial.print(F("Pressure = "));
        Serial.print(bme688.pressure / 100.0);
        Serial.println(F(" hPa"));

        Serial.print(F("Humidity = "));
        Serial.print(bme688.humidity);
        Serial.println(F(" %"));

        Serial.print(F("Gas = "));
        Serial.print(bme688.gas_resistance / 1000.0);
        Serial.println(F(" KOhms"));

        Serial.print(F("Approx. Altitude = "));
        Serial.print(bme688.readAltitude(SEALEVELPRESSURE_HPA));
        Serial.println(F(" m"));

        Serial.println();
    }
    
    void loop(){
        
    }

    char getName() {
        return *name;
    }

    DynamicJsonDocument getData()
    {
        // Mesaure temp, pressure, humidity with BMP688
        unsigned long endTime = bme688.beginReading();
        if (!bme688.endReading())
        {
            Serial.println(F("Failed to complete bme688 :("));
        }

        DynamicJsonDocument doc(200);
        doc["temperature_bme688"] = bme688.temperature;
        doc["pressure_bme688"] = bme688.pressure / 100.0;
        doc["humidity_bme688"] = bme688.humidity;
        doc["gas_resistance_bme688"] = bme688.gas_resistance / 1000.0;
        return doc;
    }
};

