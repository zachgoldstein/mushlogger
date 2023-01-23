/*
Setup core components
- External RTC 
- SD card
- Logging frequency control

*/

#include <core.h>

#include "Arduino.h"
#include <SD.h>
#include <Wire.h>
#include <SPI.h>
#include <cstdint>

#include "RTClib.h"
#include <ArduinoJson.h>

RTC_DS3231 rtc;


const char *configFilename = "/config.json";
Config config;

// Loads the configuration from a file
bool loadConfiguration(const char *filename, Config &config)
{
    // Open file for reading
    File file = SD.open(filename);

    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/v6/assistant to compute the capacity.
    StaticJsonDocument<512> doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.println(F("Failed to read file"));
        Serial.print("deserializeJson() returned ");
        Serial.println(error.c_str());
        return false;
    }

    // Copy values from the JsonDocument to the Config
    config.timestamp = doc["timestamp"] | 1667840687;
    config.logFreq = doc["logFreq"] | 5;
    strlcpy(config.name,
            doc["name"],
            sizeof(config.name));
    strlcpy(config.mDNSName,
            doc["mDNSName"],
            sizeof(config.mDNSName));
    strlcpy(config.ssid,
            doc["ssid"],
            sizeof(config.mDNSName));
    strlcpy(config.password,
            doc["password"],
            sizeof(config.mDNSName));

    // Close the file (Curiously, File's destructor doesn't close the file)
    file.close();
    return true;
}

// Prints the content of a file to the Serial
void printFile(const char *filename)
{
    // Open file for reading
    File file = SD.open(filename);
    if (!file)
    {
        Serial.println(F("Failed to read file"));
        return;
    }

    // Extract each characters by one by one
    while (file.available())
    {
        Serial.print((char)file.read());
    }
    Serial.println();

    // Close the file
    file.close();
}

bool setupCore(int chipSelect) {

    Serial.println("Setting up SD card");
    // see if the card is present and can be initialized:
    if (!SD.begin(chipSelect))
    {
        Serial.println("Card failed, or not present");
        // don't do anything more:
        return false;
    }
    Serial.println("Finished setting up SD card");

    Serial.println(String("Loading config at ") + configFilename);
    if (!loadConfiguration(configFilename, config))
    {
        Serial.println("Failed loading config");
        // don't do anything more:
        return false;
    }
    printFile(configFilename);
    Serial.println("Finished loading config");

    Serial.println("Setting up RTC");
    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        // return false;
    }
    // if (rtc.lostPower())
    // {
    //     Serial.println("RTC lost power, let's set the time!");
    //     // When time needs to be set on a new device, or after a power loss, the
    //     // following line sets the RTC to the date & time this sketch was compiled
    //     // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    //     // This line sets the RTC with an explicit date & time, for example to set
    //     // January 21, 2014 at 3am you would call:
    //     // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

    //     rtc.adjust(DateTime(config.timestamp));
    // }
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("Finished setting up RTC");

    return true;
}

void loopCore() {

}

uint32_t getUnixtime()
{
    DateTime now = rtc.now();
    return now.unixtime();
}