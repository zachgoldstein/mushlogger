#include <sensors/base_sensor.h>
#include "Arduino.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <cstdint>

#include "Adafruit_TSL2591.h"
#include <ArduinoJson.h>

class TSL2591Sensor : public BaseSensor
{
    private:
        Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591); // pass in a number for the sensor identifier (for your use later)
        String name = "TSL2591 Lux sensor";

    public:
        bool setup()
        {
            Serial.println("Setting up TSL2591 light sensor");

            // You can change the gain on the fly, to adapt to brighter/dimmer light situations
            // tsl.setGain(TSL2591_GAIN_LOW);    // 1x gain (bright light)
            tsl.setGain(TSL2591_GAIN_MED); // 25x gain
            // tsl.setGain(TSL2591_GAIN_HIGH);   // 428x gain

            // Changing the integration time gives you a longer time over which to sense light
            // longer timelines are slower, but are good in very low light situtations!
            // tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);  // shortest integration time (bright light)
            // tsl.setTiming(TSL2591_INTEGRATIONTIME_200MS);
            tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
            // tsl.setTiming(TSL2591_INTEGRATIONTIME_400MS);
            // tsl.setTiming(TSL2591_INTEGRATIONTIME_500MS);
            // tsl.setTiming(TSL2591_INTEGRATIONTIME_600MS);  // longest integration time (dim light)

            /* Display the gain and integration time for reference sake */
            Serial.println(F("------------------------------------"));
            Serial.print(F("Gain:         "));
            tsl2591Gain_t gain = tsl.getGain();
            switch (gain)
            {
            case TSL2591_GAIN_LOW:
                Serial.println(F("1x (Low)"));
                break;
            case TSL2591_GAIN_MED:
                Serial.println(F("25x (Medium)"));
                break;
            case TSL2591_GAIN_HIGH:
                Serial.println(F("428x (High)"));
                break;
            case TSL2591_GAIN_MAX:
                Serial.println(F("9876x (Max)"));
                break;
            }
            Serial.print(F("Timing:       "));
            Serial.print((tsl.getTiming() + 1) * 100, DEC);
            Serial.println(F(" ms"));
            Serial.println(F("------------------------------------"));
            Serial.println(F(""));

            Serial.println("Finished setting up TSL2591 light sensor");
            return true;
        }

        void debug()
        {
            sensor_t sensor;
            tsl.getSensor(&sensor);
            Serial.println(F("------------------------------------"));
            Serial.print(F("Sensor:       "));
            Serial.println(sensor.name);
            Serial.print(F("Driver Ver:   "));
            Serial.println(sensor.version);
            Serial.print(F("Unique ID:    "));
            Serial.println(sensor.sensor_id);
            Serial.print(F("Max Value:    "));
            Serial.print(sensor.max_value);
            Serial.println(F(" lux"));
            Serial.print(F("Min Value:    "));
            Serial.print(sensor.min_value);
            Serial.println(F(" lux"));
            Serial.print(F("Resolution:   "));
            Serial.print(sensor.resolution, 4);
            Serial.println(F(" lux"));
            Serial.println(F("------------------------------------"));
            Serial.println(F(""));

            // TSL2591 Light Sensor
            uint32_t lum = tsl.getFullLuminosity();
            uint16_t ir, full;
            ir = lum >> 16;
            full = lum & 0xFFFF;
            float lux = tsl.calculateLux(full, ir);

            Serial.print(F("[ "));
            Serial.print(millis());
            Serial.print(F(" ms ] "));
            Serial.print(F("IR: "));
            Serial.print(ir);
            Serial.print(F("  "));
            Serial.print(F("Full: "));
            Serial.print(full);
            Serial.print(F("  "));
            Serial.print(F("Visible: "));
            Serial.print(full - ir);
            Serial.print(F("  "));
            Serial.print(F("Lux: "));
            Serial.println(lux, 6);
        }

        void loop()
        {
        }

        String getName()
        {
            return name;
        }

        DynamicJsonDocument getData()
        {
            uint32_t lum = tsl.getFullLuminosity();
            uint16_t ir, full;
            ir = lum >> 16;
            full = lum & 0xFFFF;
            float lux = tsl.calculateLux(full, ir);

            DynamicJsonDocument doc(200);
            doc["visible_luminosity_tsl2591"] = full - ir;
            doc["infrared_luminosity_tsl2591"] = ir;
            doc["lux_tsl2591"] = lux;
            return doc;
        }
};
