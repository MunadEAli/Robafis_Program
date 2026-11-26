// Step 1 - smoke test: proves the toolchain, upload and serial monitor all work.
#include <Arduino.h>
#include "config.h"
#include "core/geometry.h"

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(cfg::SERIAL_BAUD);
    Serial.println(F("ColonyAFIS smoke test: MegaPi alive"));

    Serial.print(F("Envelope check - width fits: "));
    Serial.print(geometry::widthFits(cfg::ROBOT_WIDTH_CM, cfg::MAX_WIDTH_CM) ? F("yes") : F("NO"));
    Serial.print(F(", turn radius: "));
    Serial.print(geometry::turnRadius(cfg::ROBOT_WIDTH_CM, cfg::ROBOT_FRONT_CM, cfg::ROBOT_REAR_CM));
    Serial.print(F(" cm (max "));
    Serial.print(cfg::MAX_TURN_RADIUS_CM);
    Serial.println(F(" cm)"));
}

void loop() {
    static uint32_t last = 0;
    const uint32_t now = millis();
    if (now - last >= 500) {
        last = now;
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        Serial.print(F("t = "));
        Serial.print(now);
        Serial.println(F(" ms"));
    }
}
