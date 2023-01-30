/*

Mushroom Datalogger

*/

// App functions
#include <server.h>
#include <sensor.h>
#include <core.h>

// Library deps
#include "Arduino.h"
#include <Wire.h>
#include <SPI.h>
#include <cstdint>
#include <string>

const byte PICO_I2C_SDA = 12;
const byte PICO_I2C_SCL = 13;

const int chipSelect = 17;

const int loopDelay = 100;

void fatal()
{
  Serial.println("Fatal problem, freezing");
  // rp2040.reboot();
  while (1)
    delay(10);
}

void setup()
{
  // pico-specific setup
  Serial.begin(115200);
  // Delay for a bit of time to start the serial monitor
  delay(3000);

  Serial.println( "Compiled: " __DATE__ ", " __TIME__ ", " __VERSION__);

  // pico-specific setup for i2c. Lots of possible pins for i2c
  Wire.setSDA(PICO_I2C_SDA);
  Wire.setSCL(PICO_I2C_SCL);
  Wire.begin();

  if (!setupCore(chipSelect)) {
    fatal();
  }

  if (!setupSensors())
  {
    fatal();
  }
  if (!setupServer())
  {
    fatal();
  };
}

void loop()
{
  uint32_t now = getUnixtime();
  loopCore();
  String data_location = "/data_raw.jsonfiles";
  loopSensors(now, data_location);
  loopServer();

  delay(loopDelay);
}