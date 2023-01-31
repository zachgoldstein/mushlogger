#include <ArduinoJson.h>
#include "Arduino.h"
#pragma once

class BaseSensor {
    public:
    virtual String getName() = 0;
    virtual void debug() = 0;
    virtual bool setup() = 0;
    virtual void loop() = 0;
    virtual DynamicJsonDocument getData() = 0;
};
