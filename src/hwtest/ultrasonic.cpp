// Desk test 4 (plan step 4): the side ultrasonic on the real MegaPi.
//
//   Upload:  pio run -e hw_ultrasonic -t upload
//   Monitor: pio device monitor -e hw_ultrasonic       (letters are sent as you type)
//
// Live mode (on at start): 20 readings a second, one CSV line each:
//   R,<t ms>,<cm>,<read us>        cm = 0 means no echo within cfg::US_MAX_CM
// Step mode, to measure the apparent width of a cube or a box: keep the sensor still, put the object
// at the start of the ruler, type z (position 0), then for each 0.5 cm step type m. Each m takes
// cfg::US_STEP_SAMPLES readings at this position, prints one line, and moves the position on:
//   S,<x cm>,<median cm>,<min cm>,<max cm>,<valid readings>
// The width the sensor "sees" is the x range where the median drops to the object's distance.
//
// Commands: g live on/off, m measure here and step on, u step back, z position = 0, v version, h help
// Lines starting with '#' are comments: a saved log reads as CSV once they are skipped.
//
// Why not Makeblock's distanceCm(60): in library 3.31 it ignores the 60 and waits for the sensor's
// whole echo pulse (tens of ms with nothing in range), and it repeats the previous value if called
// again within 23 ms. This reads the pin directly with a real cfg::US_TIMEOUT_US limit (3.7 ms).

#include <Arduino.h>
#include "config.h"
#include "desk_io.h"

namespace {

Print& out = desk::out();

bool     live          = true;
uint8_t  usPort        = cfg::US_PORT;  // keys 5-8 pick another port (diagnosis)
uint8_t  usPin         = cfg::US_PIN;   // 'x' swaps to the port's other pin (diagnosis)
uint32_t lastSampleMs  = 0;
float    x             = 0.0f;    // ruler position in step mode, cm
uint8_t  stepLeft      = 0;       // readings still to take at this position
uint8_t  stepCount     = 0;
float    stepCm[cfg::US_STEP_SAMPLES];

// One ping on `pin`. Returns the echo time in us, 0 if no echo before `timeoutUs`.
// Blocks at most timeoutUs plus the 12 us trigger pulse.
uint32_t ping(uint8_t pin, uint32_t timeoutUs) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delayMicroseconds(2);
    digitalWrite(pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(pin, LOW);
    pinMode(pin, INPUT);
    return pulseIn(pin, HIGH, timeoutUs);
}

void printHelp() {
    out.println(F("# R,t_ms,cm,read_us = live reading (cm 0 = no echo)"));
    out.println(F("# S,x_cm,median_cm,min_cm,max_cm,valid = one ruler position in step mode"));
    out.println(F("# Commands: g live on/off, m measure here and step on, u step back, z position = 0, v version, h help"));
    out.println(F("# Diagnosis: 5-8 pick the port, d test both pins of the port, x switch the measuring pin"));
}

// Diagnosis: for both pins of the port, the resting level and one ping with a long wait
// (30 ms, like Makeblock's own code). Blocks up to about 60 ms: only on request.
void diagnose(uint32_t now) {
    const uint8_t pins[2] = {cfg::MEGAPI_PORT_PIN1[usPort], cfg::MEGAPI_PORT_PIN2[usPort]};
    out.print(F("# DIAG port "));
    out.println(usPort);
    for (uint8_t i = 0; i < 2; ++i) {
        pinMode(pins[i], INPUT);
        delayMicroseconds(50);
        const int idle = digitalRead(pins[i]);
        const uint32_t echoUs = ping(pins[i], 30000UL);
        out.print(F("# DIAG pin "));
        out.print(pins[i]);
        out.print(F(": resting level "));
        out.print(idle);
        out.print(F(", echo "));
        out.print(echoUs);
        out.print(F(" us = "));
        out.print(echoUs / 58.0f, 1);
        out.println(F(" cm"));
        delay(30);   // let the sensor settle before the next ping (diagnosis only)
    }
    out.print(F("# measuring pin is "));
    out.print(usPin);
    desk::endReply(now);
}

void printPosition(uint32_t now) {
    out.print(F("# position x = "));
    out.print(x, 1);
    out.print(F(" cm"));
    desk::endReply(now);
}

// Sorts the few step readings (insertion sort) and prints the S line.
void finishStep() {
    float v[cfg::US_STEP_SAMPLES];
    uint8_t n = 0;
    for (uint8_t i = 0; i < stepCount; ++i) {
        if (stepCm[i] <= 0.0f) continue;       // no echo: not part of the median
        uint8_t j = n++;
        while (j > 0 && v[j - 1] > stepCm[i]) { v[j] = v[j - 1]; --j; }
        v[j] = stepCm[i];
    }
    out.print(F("S,"));
    out.print(x, 1);
    out.print(',');
    out.print(n ? v[n / 2] : 0.0f, 1);
    out.print(',');
    out.print(n ? v[0] : 0.0f, 1);
    out.print(',');
    out.print(n ? v[n - 1] : 0.0f, 1);
    out.print(',');
    out.println(n);
    x += cfg::US_STEP_CM;
}

bool handle(char c, uint32_t now) {
    switch (c) {
        case 'g':
            live = !live;
            out.print(live ? F("# live ON") : F("# live OFF"));
            desk::endReply(now);
            return true;
        case 'm':
            if (stepLeft == 0) {               // ignore a second m while still measuring
                stepLeft  = cfg::US_STEP_SAMPLES;
                stepCount = 0;
            }
            return true;
        case 'u':
            x -= cfg::US_STEP_CM;
            printPosition(now);
            return true;
        case 'z':
            x = 0.0f;
            printPosition(now);
            return true;
        case 'v':
            out.print(F("# VER," CFG_FW_VERSION ",hw_ultrasonic," __DATE__ " " __TIME__));
            desk::endReply(now);
            return true;
        case 'd':
            diagnose(now);
            return true;
        case 'x':
            usPin = (usPin == cfg::MEGAPI_PORT_PIN1[usPort]) ? cfg::MEGAPI_PORT_PIN2[usPort]
                                                             : cfg::MEGAPI_PORT_PIN1[usPort];
            pinMode(usPin, INPUT);
            out.print(F("# measuring pin is now "));
            out.print(usPin);
            desk::endReply(now);
            return true;
        case '5':
        case '6':
        case '7':
        case '8':
            usPort = static_cast<uint8_t>(c - '0');
            usPin  = cfg::MEGAPI_PORT_PIN2[usPort];   // the sensor's signal is normally the second pin
            pinMode(usPin, INPUT);
            out.print(F("# port "));
            out.print(usPort);
            out.print(F(", measuring pin "));
            out.print(usPin);
            desk::endReply(now);
            return true;
        case 'h':
        case '?':
            printHelp();
            return true;
        default:
            return false;
    }
}

}  // namespace

void setup() {
    desk::begin();
    pinMode(cfg::US_PIN, INPUT);

    out.println();
    out.print(F("# ColonyAFIS hw_ultrasonic " CFG_FW_VERSION ": desk test 4, side ultrasonic on port "));
    out.print(cfg::US_PORT);
    out.print(F(" (pin "));
    out.print(cfg::US_PIN);
    out.print(F("), no echo beyond "));
    out.print(cfg::US_MAX_CM);
    out.println(F(" cm"));
    printHelp();
    lastSampleMs = millis();
}

void loop() {
    const uint32_t now = millis();
    desk::poll(now, handle);

    if (now - lastSampleMs < cfg::US_DESK_PERIOD_MS) return;
    lastSampleMs = now;

    const uint32_t t0 = micros();
    const uint32_t echoUs = ping(usPin, cfg::US_TIMEOUT_US);
    const uint32_t readUs = micros() - t0;
    const float cm = echoUs / 58.0f;          // sound: 58 us per cm there and back

    if (live) {
        out.print(F("R,"));
        out.print(now);
        out.print(',');
        out.print(cm, 1);
        out.print(',');
        out.println(readUs);
    }
    if (stepLeft > 0) {
        stepCm[stepCount++] = cm;
        if (--stepLeft == 0) finishStep();
    }
}
