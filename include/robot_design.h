#pragma once
// =====================================================================================
//  ROBOT DESIGN - the mechanical numbers the software is built on.
//  Owner: the mechanical designer. Copy each value from the CAD.
//
//  How to edit: change ONLY the numbers. Keep the "f" and the ";" after each number.
//  After every change, the automatic check on GitHub (or `pio test -e test`) says
//  whether the design still meets the competition limits, and why not if it fails.
//  Guide with a drawing: docs/mechanical_interface.md
//
//  Robot frame (top view):
//    origin = middle of the wheel axle
//    +x     = forward (the end with the line sensor; the robot scans moving this way)
//    +y     = to the robot's LEFT (the side with the ultrasonic and the arm)
//  Units: cm. All values are TBD until the CAD is final.
// =====================================================================================

namespace cfg {

// ---------- Outer body: everything included (wheels, arm retracted, cables, screws) ----------
constexpr float ROBOT_WIDTH_CM = 16.0f;   // total width, left-most to right-most point    [limit: <= 16]
constexpr float ROBOT_FRONT_CM = 9.0f;    // axle -> front-most point                       [limit: turning radius]
constexpr float ROBOT_REAR_CM  = 9.0f;    // axle -> rear-most point                        [limit: turning radius]

// ---------- Wheels ----------
constexpr float WHEEL_DIAMETER_CM = 6.4f;   // drive wheel outer diameter
constexpr float WHEEL_BASE_CM     = 13.0f;  // distance between the centres of the two wheels' treads

// ---------- Line follower (under the robot) ----------
constexpr float LINE_SENSOR_X_CM = 6.0f;    // axle -> middle of the two sensors, forward    [limit: > 0, inside the body]
constexpr float LINE_SENSOR_Y_CM = 0.0f;    // sideways offset from the centreline           [limit: on the centreline]

// ---------- Ultrasonic (left side, facing the deposits) ----------
constexpr float ULTRASONIC_X_CM = 7.0f;     // axle -> sensor axis, forward                  [limit: >= 6 ahead of the arm]
constexpr float ULTRASONIC_Y_CM = 8.0f;     // centreline -> sensor front face, to the left  [limit: inside the body]

// ---------- Arm (rack and pinion, extends to the LEFT; colour sensor and gripper at the tip) ----------
// "Grip point" = the middle of the gripper jaws, where the cube's centre sits when it is held.
constexpr float ARM_X_CM                = 0.0f;   // axle -> arm axis, forward (negative = behind the axle)
constexpr float ARM_GRIP_RETRACTED_Y_CM = 6.5f;   // centreline -> grip point, arm fully retracted   [limit: inside the body]
constexpr float ARM_STROKE_CM           = 15.0f;  // rack travel, fully retracted -> fully extended  [limit: reach]

// ---------- IMU (MPU-6050 board) ----------
constexpr float IMU_X_CM = 0.0f;            // axle -> IMU chip, forward. Keep near the axle centre,
constexpr float IMU_Y_CM = 0.0f;            // centreline -> IMU chip, left. board x-axis pointing forward, rigid mount.

}  // namespace cfg
