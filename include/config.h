#pragma once
// ColonyAFIS - every tunable number lives here, in one place.
// Units: cm, seconds, degrees unless stated. "TBD" = replace when the real robot exists.

#include <stdint.h>

// The robot's mechanical dimensions (size, wheels, arm, sensor positions) live in their own
// file, owned by the mechanical designer. They are part of cfg like everything else.
#include "robot_design.h"

namespace cfg {

// ---------- Firmware identity ----------
// A macro, not a constexpr array, so the robot can print it with F(...) and keep it in flash
// instead of SRAM.
#define CFG_FW_VERSION "0.1.0"

// ---------- Serial / Bluetooth ----------
constexpr uint32_t SERIAL_BAUD = 115200;   // USB serial monitor
// Bluetooth: Makeblock dual-mode module in the MegaPi's onboard wireless socket, Android phone.
// Desk test 1 (hw_link, 26 Nov 2026): the module's bytes arrive on Serial3 as clean text at 115200.
// While no phone is connected the module answers what it receives with "ERROR01" (it reads it as
// setup commands), so the code must never echo unknown Bluetooth bytes back to Bluetooth.
constexpr uint8_t  BT_SERIAL_PORT = 3;      // Serial3 = pins 14/15 (confirmed)
constexpr uint32_t BT_BAUD        = 115200; // confirmed

// ---------- MegaPi sensor ports (RJ25 sockets 5-8 on the "MegaPi Shield for RJ25") ----------
// Fixed board wiring, from Makeblock's MeMegaPi.cpp (mePort table): each port has two signal pins.
// Port 5 = 16/17 (also Serial2), 6 = A8/A9, 7 = A10/A11, 8 = A13/A12. Arduino Mega: A0 = 54.
constexpr uint8_t MEGAPI_PORT_PIN1[9] = {0, 0, 0, 0, 0, 16, 62, 64, 67};  // index = port number
constexpr uint8_t MEGAPI_PORT_PIN2[9] = {0, 0, 0, 0, 0, 17, 63, 65, 66};

// ---------- Side ultrasonic (Me Ultrasonic, one signal pin = the port's second pin) ----------
// Desk test 4 (27 Nov 2026): port 6, signal on its second pin (63 = A9). Measured 10.2 cm at 10 cm
// and 30.0 cm at 30 cm. Port 5 (the shield's serial port, via its 2x2 socket) gave no echo on either
// pin: 15-23 us blips at 10 cm, pin 17 resting HIGH. Suspect the 2x2 corner of the shield.
constexpr uint8_t  US_PORT          = 6;
constexpr uint8_t  US_PIN           = MEGAPI_PORT_PIN2[US_PORT];
constexpr uint16_t US_MAX_CM        = 60;    // farther echoes count as "no echo"
constexpr uint32_t US_TIMEOUT_US    = US_MAX_CM * 58UL + 200UL;  // 3680 us: the longest a read may block
constexpr uint16_t US_DESK_PERIOD_MS = 50;   // desk test 4: 20 readings a second
constexpr uint8_t  US_STEP_SAMPLES  = 5;     // readings per ruler position in step mode (median kept)
constexpr float    US_STEP_CM       = 1.0f;  // ruler step between positions in step mode

// ---------- SCAN: cube or robot by apparent width (desk test 4, tests/results/2026-11-27_ultrasonic.csv) ----------
// Measured, sensor still: a 3 cm cube looks 7-9 cm wide at 8-14 cm; a 16 cm flat object looks ~23 cm wide
// at 10 cm (the echo reaches ~3.5 cm past each end). Starting values, to re-check while driving (I2).
constexpr float SCAN_CUBE_MAX_WIDTH_CM  = 12.0f;  // up to this: CUBE (measured max 9 + margin)
constexpr float SCAN_ROBOT_MIN_WIDTH_CM = 16.0f;  // from this: ROBOT; in between: OBSTACLE, don't grab
constexpr float SCAN_MIN_VALID_CM       = 3.0f;   // plan: shorter readings are not trusted
// Edge readings come out up to ~5 cm too long, so an object's distance is its closest readings, not the mean.

// ---------- Colour sensor (Makeblock Me Color Sensor V1.0 = ROHM BH1745, I2C) ----------
// Needs a port with a white (I2C) patch: 6, 7 or 8. The port's two plain pins switch the board
// (Makeblock MeColorSensor.cpp): S1 = pin 1, HIGH = white LEDs on; S2 = pin 2, LOW = sensor powered.
constexpr uint8_t  COLOR_PORT       = 8;      // plugged in by Munad, 27 Nov 2026
constexpr uint8_t  COLOR_LED_PIN    = MEGAPI_PORT_PIN1[COLOR_PORT];   // 67 (A13)
constexpr uint8_t  COLOR_POWER_PIN  = MEGAPI_PORT_PIN2[COLOR_PORT];   // 66 (A12), LOW = on
constexpr uint8_t  COLOR_I2C_ADDR   = 0x38;
constexpr uint16_t COLOR_PERIOD_MS  = 200;    // the chip measures every 160 ms; read a little slower
constexpr uint8_t  COLOR_SAMPLES    = 20;     // plan: each cube and card read 20 times
constexpr uint32_t I2C_TIMEOUT_US   = 3000;   // a stuck I2C bus gives up instead of hanging the loop

// ---------- Link supervision ----------
constexpr uint16_t HEARTBEAT_PERIOD_MS = 500;   // plan: HB twice a second
constexpr uint16_t STOP_LATENCY_MAX_MS = 500;   // desk test 1: phone STOP must show up within this
constexpr uint16_t CMD_GAP_MS          = 30;    // a letter is a command only at a line start or after
                                                // this much silence (typing); module chatter is faster

// ---------- Limits the mechanical design must meet (plan, "Envelope for the mechanical design") ----------
// Checked automatically against robot_design.h by test/test_robot_design.
constexpr float MAX_WIDTH_CM                = 16.0f;  // line corridor ~20 cm; both sides face deposits on some trips
constexpr float MAX_TURN_RADIUS_CM          = 12.5f;  // = sqrt(g^2 + (gap/2)^2): 180-degree turn between two deposits
constexpr float MIN_US_AHEAD_OF_ARM_CM      = 6.0f;   // cube echo ends before the arm passes it: no reversing
constexpr float LINE_SENSOR_MAX_OFFSET_Y_CM = 0.5f;   // "on the centreline"
constexpr float CUBE_SIZE_CM                = 3.0f;   // resource cube edge

// ---------- Drive (TBD: measure on the robot) ----------
constexpr float ENC_PULSES_PER_REV = 8.0f;    // encoder pulses per motor-shaft turn
constexpr float DRIVE_GEAR_RATIO   = 46.67f;  // 185 rpm motors (check motor label)
constexpr float ARM_GEAR_RATIO     = 75.0f;   // 86 rpm motor (check motor label)

// ---------- Timing ----------
constexpr uint16_t CONTROL_PERIOD_MS = 10;    // main control loop, 100 Hz

// ---------- Track, team frame (cm) ----------
constexpr float TRACK_W_CM    = 130.0f;
constexpr float TRACK_H_CM    = 110.0f;
constexpr float COLONY_LINE_Y = 35.0f;

constexpr float DEPOSIT_X_MIN[3] = {20.0f, 55.0f, 90.0f};
constexpr float DEPOSIT_X_MAX[3] = {40.0f, 75.0f, 110.0f};
constexpr float DEPOSIT_Y_MIN    = 45.0f;
constexpr float DEPOSIT_Y_MAX    = 65.0f;

constexpr float ZONE_X[5]   = {20.0f, 42.5f, 65.0f, 87.5f, 110.0f};  // OBS HAB FARM IND REC
constexpr float ZONE_Y_MIN  = 10.0f;
constexpr float ZONE_Y_MAX  = 25.0f;

constexpr float TURN_SPOT_X[2]  = {47.5f, 82.5f};
constexpr float FENCE_MARGIN_CM = 3.0f;       // allowed x range: [R + 3, 130 - R - 3]

}  // namespace cfg
