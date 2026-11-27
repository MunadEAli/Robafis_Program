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
