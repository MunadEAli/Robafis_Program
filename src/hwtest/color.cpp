// Desk test 6 (plan step 6): the colour sensor on the real MegaPi.
// Makeblock Me Color Sensor V1.0 = ROHM BH1745 (red, green, blue and clear channels) over I2C.
//
//   Upload:  pio run -e hw_color -t upload
//   Monitor: pio device monitor -e hw_color          (letters are sent as you type)
//
// Live mode (on at start), about 5 lines a second:
//   L,<t ms>,<R>,<G>,<B>,<C>                 raw 16-bit counts, C = clear (overall brightness)
// m measures the object in front of the sensor cfg::COLOR_SAMPLES times. Every reading is printed,
// then one summary line, the colour's "fingerprint" with the brightness divided out:
//   C,<k>,<i>,<R>,<G>,<B>,<C>                reading i of measurement k
//   M,<k>,<n>,<R>,<G>,<B>,<C>,<r>,<g>,<b>,<r sd>,<g sd>,<b sd>,<C min>,<C max>
//     r = R / C (likewise g, b), averaged over the n readings; sd = how much it varied.
//
// Commands: g live on/off, m measure 20 readings, l LEDs on/off, i I2C scan, v version, h help
// Lines starting with '#' are comments: a saved log reads as CSV once they are skipped.

#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "config.h"
#include "desk_io.h"

namespace {

// BH1745 registers (datasheet; same values as Makeblock's MeColorSensor.h).
constexpr uint8_t REG_SYSTEM_CONTROL = 0x40;
constexpr uint8_t REG_MODE_CONTROL1  = 0x41;
constexpr uint8_t REG_MODE_CONTROL2  = 0x42;
constexpr uint8_t REG_MODE_CONTROL3  = 0x44;
constexpr uint8_t REG_RED_LSB        = 0x50;   // R, G, B, C: 8 bytes, little-endian pairs
constexpr uint8_t REG_MANUFACTURER   = 0x92;
constexpr uint8_t MANUFACTURER_ROHM  = 0xE0;

Print& out = desk::out();

bool     live       = true;
bool     ledsOn     = true;
bool     sensorOk   = false;
uint32_t lastReadMs = 0;

// Measurement in progress (m).
uint8_t  measureLeft = 0;
uint16_t measureK    = 0;
uint8_t  nOk         = 0;
float    sumR, sumG, sumB, sumC, sqR, sqG, sqB;   // ratios r, g, b (and raw C)
float    rawR, rawG, rawB;                        // raw counts, for the raw means
uint16_t cMin, cMax;

bool writeReg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(cfg::COLOR_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

bool readRegs(uint8_t start, uint8_t* buf, uint8_t n) {
    Wire.beginTransmission(cfg::COLOR_I2C_ADDR);
    Wire.write(start);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(cfg::COLOR_I2C_ADDR, n) != n) return false;
    for (uint8_t i = 0; i < n; ++i) buf[i] = Wire.read();
    return true;
}

// Powers the board, checks the chip answers, and starts continuous 160 ms measurements.
bool initSensor() {
    digitalWrite(cfg::COLOR_POWER_PIN, LOW);     // sensor on
    digitalWrite(cfg::COLOR_LED_PIN, ledsOn ? HIGH : LOW);
    delay(10);                                   // setup only: let the chip power up

    uint8_t id = 0;
    if (!readRegs(REG_MANUFACTURER, &id, 1)) {
        out.println(F("# colour sensor: no answer at I2C 0x38 (try i to scan; check the cable and port)"));
        return false;
    }
    out.print(F("# colour sensor: manufacturer id 0x"));
    out.print(id, HEX);
    out.println(id == MANUFACTURER_ROHM ? F(" (ROHM BH1745, as expected)") : F(" (unexpected!)"));

    return writeReg(REG_SYSTEM_CONTROL, 0xC0)    // software reset + interrupt reset
        && writeReg(REG_MODE_CONTROL1, 0x00)     // 160 ms per measurement
        && writeReg(REG_MODE_CONTROL2, 0x10)     // measuring on, gain x1
        && writeReg(REG_MODE_CONTROL3, 0x02);    // datasheet: must be written as 0x02
}

void printHelp() {
    out.println(F("# L,t_ms,R,G,B,C = live raw reading (C = clear, overall brightness)"));
    out.println(F("# C,k,i,R,G,B,C = reading i of measurement k"));
    out.println(F("# M,k,n,R,G,B,C,r,g,b,r_sd,g_sd,b_sd,C_min,C_max = summary, r = R/C etc."));
    out.println(F("# Commands: g live on/off, m measure 20 readings, l LEDs on/off, i I2C scan, v version, h help"));
}

void scanI2c(uint32_t now) {
    out.print(F("# I2C devices found:"));
    uint8_t found = 0;
    for (uint8_t a = 1; a < 127; ++a) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) {
            out.print(F(" 0x"));
            out.print(a, HEX);
            ++found;
        }
    }
    if (found == 0) out.print(F(" none"));
    desk::endReply(now);
}

void startMeasure() {
    ++measureK;
    measureLeft = cfg::COLOR_SAMPLES;
    nOk = 0;
    sumR = sumG = sumB = sumC = sqR = sqG = sqB = 0.0f;
    rawR = rawG = rawB = 0.0f;
    cMin = 0xFFFF;
    cMax = 0;
}

float sd(float sumSq, float sum, uint8_t n) {
    if (n < 2) return 0.0f;
    const float mean = sum / n;
    const float var  = sumSq / n - mean * mean;
    return var > 0.0f ? sqrtf(var) : 0.0f;
}

void finishMeasure() {
    out.print(F("M,"));
    out.print(measureK);
    out.print(',');
    out.print(nOk);
    if (nOk == 0) {
        out.println(F(",,,,,,,,,,,,"));
        return;
    }
    // Raw means, then the brightness-free fingerprint r, g, b with its spread.
    out.print(',');
    out.print(rawR / nOk, 0);
    out.print(',');
    out.print(rawG / nOk, 0);
    out.print(',');
    out.print(rawB / nOk, 0);
    out.print(',');
    out.print(sumC / nOk, 0);
    out.print(',');
    out.print(sumR / nOk, 4);
    out.print(',');
    out.print(sumG / nOk, 4);
    out.print(',');
    out.print(sumB / nOk, 4);
    out.print(',');
    out.print(sd(sqR, sumR, nOk), 4);
    out.print(',');
    out.print(sd(sqG, sumG, nOk), 4);
    out.print(',');
    out.print(sd(sqB, sumB, nOk), 4);
    out.print(',');
    out.print(cMin);
    out.print(',');
    out.println(cMax);
}

bool handle(char c, uint32_t now) {
    switch (c) {
        case 'g':
            live = !live;
            out.print(live ? F("# live ON") : F("# live OFF"));
            desk::endReply(now);
            return true;
        case 'm':
            if (measureLeft == 0) {
                startMeasure();
                out.print(F("# measuring "));
                out.print(cfg::COLOR_SAMPLES);
                out.print(F(" readings, k = "));
                out.print(measureK);
                desk::endReply(now);
            }
            return true;
        case 'l':
            ledsOn = !ledsOn;
            digitalWrite(cfg::COLOR_LED_PIN, ledsOn ? HIGH : LOW);
            out.print(ledsOn ? F("# LEDs ON") : F("# LEDs OFF"));
            desk::endReply(now);
            return true;
        case 'i':
            scanI2c(now);
            if (!sensorOk) sensorOk = initSensor();   // try again after a scan
            return true;
        case 'v':
            out.print(F("# VER," CFG_FW_VERSION ",hw_color," __DATE__ " " __TIME__));
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
    pinMode(cfg::COLOR_LED_PIN, OUTPUT);
    pinMode(cfg::COLOR_POWER_PIN, OUTPUT);
    Wire.begin();
    Wire.setWireTimeout(cfg::I2C_TIMEOUT_US, true);

    out.println();
    out.print(F("# ColonyAFIS hw_color " CFG_FW_VERSION ": desk test 6, colour sensor on port "));
    out.print(cfg::COLOR_PORT);
    out.print(F(" (LED pin "));
    out.print(cfg::COLOR_LED_PIN);
    out.print(F(", power pin "));
    out.print(cfg::COLOR_POWER_PIN);
    out.println(F(")"));
    printHelp();
    sensorOk = initSensor();
    lastReadMs = millis();
}

void loop() {
    const uint32_t now = millis();
    desk::poll(now, handle);

    if (!sensorOk || now - lastReadMs < cfg::COLOR_PERIOD_MS) return;
    lastReadMs = now;

    uint8_t b[8];
    if (!readRegs(REG_RED_LSB, b, sizeof(b))) {
        out.println(F("# read failed (sensor unplugged?) - i to retry"));
        sensorOk = false;
        return;
    }
    const uint16_t R = b[0] | (b[1] << 8);
    const uint16_t G = b[2] | (b[3] << 8);
    const uint16_t B = b[4] | (b[5] << 8);
    const uint16_t C = b[6] | (b[7] << 8);

    if (measureLeft > 0) {
        out.print(F("C,"));
        out.print(measureK);
        out.print(',');
        out.print(cfg::COLOR_SAMPLES - measureLeft + 1);
    } else if (live) {
        out.print(F("L,"));
        out.print(now);
    }
    if (measureLeft > 0 || live) {
        out.print(',');
        out.print(R);
        out.print(',');
        out.print(G);
        out.print(',');
        out.print(B);
        out.print(',');
        out.println(C);
    }

    if (measureLeft > 0) {
        if (C > 0) {
            const float r = static_cast<float>(R) / C;
            const float g = static_cast<float>(G) / C;
            const float bl = static_cast<float>(B) / C;
            sumR += r;  sqR += r * r;
            sumG += g;  sqG += g * g;
            sumB += bl; sqB += bl * bl;
            sumC += C;
            rawR += R;  rawG += G;  rawB += B;
            if (C < cMin) cMin = C;
            if (C > cMax) cMax = C;
            ++nOk;
        }
        if (--measureLeft == 0) finishMeasure();
    }
}
