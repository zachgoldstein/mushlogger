#include <cstdint>
#include "Arduino.h"

void debugSensors(uint32_t timestamp);
void logSensorDataToFile(uint32_t timestamp, String data_location);
bool setupSensors();
void loopSensors(uint32_t timestamp, String data_location);
