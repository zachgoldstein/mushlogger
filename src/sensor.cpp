/*
Handle sensor setup and logging
*/

#include <core.h>

#include "Arduino.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <cstdint>

#include <ArduinoJson.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>
#include "SparkFun_SCD4x_Arduino_Library.h"
#include "Adafruit_SGP30.h"
#include "Adafruit_SGP40.h"
#include "Adafruit_BME680.h"
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"

Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591); // pass in a number for the sensor identifier (for your use later)

#define SEALEVELPRESSURE_HPA (1013.25)

extern Config config;

Adafruit_AHTX0 aht;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

Adafruit_SGP30 sgp30;
Adafruit_SGP40 sgp40;
SCD4x co2_sensor;
Adafruit_BMP280 bmp; // I2C
Adafruit_BME680 bme688; // I2C

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

const char *data_delimiter = "\n";

uint32_t lastLogTimestamp = 0;
uint32_t lastsgp30BaselineLogTimestamp = 0;
uint32_t sgp30BaselineLogFreq = 30; // Log sgp30 baseline every 30 seconds;

// Filename to log data to
extern char *DATA_FILENAME;
const char *sensorFilename = DATA_FILENAME;

void debugBME688()
{
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

void debugBMP280(){
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

void debugAHT20(){
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

void debugSCD4X(){
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

void debugSGP30(){
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

void debugSGP40()
{
    Serial.println("Debug SGP40 Sensor");
    Serial.println();
    Serial.print(F("SGP40 Sensor..."));

    Serial.print("SGP raw gas measurement");
    Serial.print(sgp40.measureRaw());
    Serial.print("SGP VOC index");
    Serial.print(sgp40.measureVocIndex());
}

void logData(uint32_t timestamp)
{
    Serial.println(String("Logging JSON Data, timestamp: ") + timestamp);

    // debugBME688(); //TEMP

    // Make JSON package with all data
    StaticJsonDocument<300> doc;
    doc["time_rtc"] = timestamp;

    // Mesaure temp, pressure, humidity with BMP688
    unsigned long endTime = bme688.beginReading();
    if (!bme688.endReading())
    {
        Serial.println(F("Failed to complete bme688 :("));
    }
    doc["temperature_bme688"] = bme688.temperature;
    doc["pressure_bme688"] = bme688.pressure / 100.0;
    doc["humidity_bme688"] = bme688.humidity;
    doc["gas_resistance_bme688"] = bme688.gas_resistance / 1000.0;

    // Measure pressure and temp with BMP280 sensor
    // doc["altitude_bmp280"] = bmp.readAltitude(1013.25);
    // doc["pressure_bmp280"] = bmp.readPressure() / 100.0;
    // doc["temperature_bmp280"] = bmp.readTemperature();

    // Measure humidty and temp with AHT20 sensor
    // sensors_event_t humidity, temp;
    // aht.getEvent(&humidity, &temp); // populate temp and humidity objects with fresh data
    // doc["temperature_aht20"] = temp.temperature;
    // doc["humidity_aht20"] = humidity.relative_humidity;

    // Measure CO2 with SCD4x sensor
    co2_sensor.readMeasurement();
    doc["CO2_SCD40"] = co2_sensor.getCO2();
    doc["temperature_SCD40"] = co2_sensor.getTemperature();
    doc["humidity_SCD40"] = co2_sensor.getHumidity();

    // Measure eCO2 and VOC with SGP30 sensor
    // sgp.setHumidity(getAbsoluteHumidity(temp.temperature, humidity.relative_humidity));
    // if (!sgp.IAQmeasure())
    // {
    //     Serial.println("SGP IAQmeasure failed");
    // }
    // doc["raw_gas_sgp40"] = sgp40.measureRaw();
    // doc["VOC_Index_sgp40"] = sgp40.measureVocIndex();

    // TSL2591 Light Sensor
    uint32_t lum = tsl.getFullLuminosity();
    uint16_t ir, full;
    ir = lum >> 16;
    full = lum & 0xFFFF;
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
    float lux = tsl.calculateLux(full, ir);
    Serial.println(lux, 6);
    doc["visible_luminosity_tsl2591"] = full - ir;
    doc["infrared_luminosity_tsl2591"] = ir;
    doc["lux_tsl2591"] = lux;

    serializeJson(doc, Serial);
    Serial.println();

    // Open file for writing
    File file = SD.open(sensorFilename, FILE_WRITE);
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

void debugTSL2591(){
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
}

void debugSensors(uint32_t timestamp)
{
    Serial.println(String("Timestamp debugSensors: ")+ timestamp);

    Serial.println();

    Serial.println();
}

bool setupSensors()
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

    // Serial.println("Setting up BPM280 pressure/temp sensor");
    // unsigned status;
    // status = bmp.begin();
    // if (!status)
    // {
    //     Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
    //                      "try a different address!"));
    //     Serial.print("SensorID was: 0x");
    //     Serial.println(bmp.sensorID(), 16);
    //     Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    //     Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    //     Serial.print("        ID of 0x60 represents a BME 280.\n");
    //     Serial.print("        ID of 0x61 represents a BME 680.\n");
    //     return false;
    // }

    // /* Default settings from datasheet. */
    // bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
    //                 Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
    //                 Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
    //                 Adafruit_BMP280::FILTER_X16,      /* Filtering. */
    //                 Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
    // Serial.println("Finished setting up BPM280 sensor");

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

    // Serial.println("Setting up the SGP30 eCO2 sensor");
    // if (!sgp.begin())
    // {
    //     Serial.println("SGP30 eCO2 Sensor not found :(");
    //     return false;
    // }
    // Serial.print("Found SGP30 serial #");
    // Serial.print(sgp.serialnumber[0], HEX);
    // Serial.print(sgp.serialnumber[1], HEX);
    // Serial.println(sgp.serialnumber[2], HEX);

    // If you have a baseline measurement from before you can assign it to start, to 'self-calibrate'
    // sgp.setIAQBaseline(0x8E68, 0x8F41); // Will vary for each sensor!
    // Serial.println("Finished setting up the SGP30 eCO2 sensor");

    // Serial.println("Setting up the SGP40 Gas Sensor");
    // if (!sgp40.begin()){
    //     Serial.println("SGP40 sensor not found :(");
    //     return false;
    // }
    // Serial.print("Found SGP40 serial #");
    // Serial.print(sgp40.serialnumber[0], HEX);
    // Serial.print(sgp40.serialnumber[1], HEX);
    // Serial.println(sgp40.serialnumber[2], HEX);

    // Serial.println("Setting up the AHT humidity/temp/pressure sensor");
    // if (!aht.begin())
    // {
    //     Serial.println("Could not find AHT? Check wiring");
    //     return false;
    // }
    // Serial.println("Finished up the AHT humidity/temp/pressure sensor");

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

void loopSensors(uint32_t timestamp)
{
    // Note: The SCD4x CO2 sensor has data ready every five seconds

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

    uint32_t timeLogDiff = timestamp - lastLogTimestamp;
    if (timeLogDiff > config.logFreq)
    {
        logData(timestamp);
        lastLogTimestamp = timestamp;
    }
}
