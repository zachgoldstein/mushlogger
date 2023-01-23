#include "Arduino.h"

void blink_leds() {
    // turn the LED on (HIGH is the voltage level)
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("ON");
    // wait for a second
    delay(1000);

    // turn the LED off by making the voltage LOW
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("OFF");
    // wait for a second
    delay(1000);
}