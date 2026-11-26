// Checks the mechanical design (include/robot_design.h) against the competition limits.
// Run on the PC with:  pio test -e test -v      (-v also prints the design report)
// It also runs automatically on GitHub after every push.
// Every failure message says which rule broke, by how much, and what to change.
#include <stdio.h>
#include <unity.h>
#include "config.h"
#include "core/geometry.h"

using namespace cfg;

void setUp() {}
void tearDown() {}

static char msg[256];
#define MSG(...) (snprintf(msg, sizeof msg, __VA_ARGS__), msg)

static const char* const ZONE_NAME[5]    = {"observation", "habitat", "farm", "industry", "recycling"};
static const char* const DEPOSIT_NAME[3] = {"left", "middle", "right"};

// ---------- Values derived from the design and the field ----------

static float turnR()    { return geometry::turnRadius(ROBOT_WIDTH_CM, ROBOT_FRONT_CM, ROBOT_REAR_CM); }
static float fMin()     { return geometry::fenceMin(turnR(), FENCE_MARGIN_CM); }
static float fMax()     { return geometry::fenceMax(TRACK_W_CM, turnR(), FENCE_MARGIN_CM); }
static float reach()    { return geometry::armReachBeyondSide(ARM_GRIP_RETRACTED_Y_CM, ARM_STROKE_CM, ROBOT_WIDTH_CM); }
// Distances from the colony line to a centred deposit cube (20 cm) and to a zone centre (17.5 cm).
static float depositDist() { return (DEPOSIT_Y_MIN + DEPOSIT_Y_MAX) * 0.5f - COLONY_LINE_Y; }
static float zoneDist()    { return COLONY_LINE_Y - (ZONE_Y_MIN + ZONE_Y_MAX) * 0.5f; }
static float needReach()     { return geometry::requiredArmReach(depositDist(), ROBOT_WIDTH_CM); }
static float needZoneReach() { return geometry::requiredArmReach(zoneDist(), ROBOT_WIDTH_CM); }
static float cubeMinX(int d) { return DEPOSIT_X_MIN[d] + CUBE_SIZE_CM * 0.5f; }
static float cubeMaxX(int d) { return DEPOSIT_X_MAX[d] - CUBE_SIZE_CM * 0.5f; }

// ---------- Report (always passes; read it to see how much room is left) ----------

void test_print_design_report() {
    const float maxLen = geometry::maxHalfLength(ROBOT_WIDTH_CM, MAX_TURN_RADIUS_CM);

    // Allowed range for ARM_X_CM so every zone (placed after the 180-degree turn) and every
    // cube position in every deposit (picked facing +x) can be reached inside the fence.
    float armLo = -1e9f, armHi = 1e9f;
    for (int z = 0; z < 5; ++z) {
        if (fMin() - ZONE_X[z] > armLo) armLo = fMin() - ZONE_X[z];
        if (fMax() - ZONE_X[z] < armHi) armHi = fMax() - ZONE_X[z];
    }
    for (int d = 0; d < 3; ++d) {
        if (cubeMaxX(d) - fMax() > armLo) armLo = cubeMaxX(d) - fMax();
        if (cubeMinX(d) - fMin() < armHi) armHi = cubeMinX(d) - fMin();
    }
    const float usLimit = ULTRASONIC_X_CM - MIN_US_AHEAD_OF_ARM_CM;
    if (usLimit < armHi) armHi = usLimit;

    TEST_MESSAGE(MSG("DESIGN REPORT  width %.1f cm, front %.1f cm, rear %.1f cm",
                     ROBOT_WIDTH_CM, ROBOT_FRONT_CM, ROBOT_REAR_CM));
    TEST_MESSAGE(MSG("turning radius %.2f cm (limit %.2f, room left %.2f)",
                     turnR(), MAX_TURN_RADIUS_CM, MAX_TURN_RADIUS_CM - turnR()));
    TEST_MESSAGE(MSG("at this width, front and rear may each be up to %.2f cm from the axle", maxLen));
    TEST_MESSAGE(MSG("grip point reaches %.2f cm beyond the side (deposit cube needs %.2f, "
                     "room left %.2f)", reach(), needReach(), reach() - needReach()));
    TEST_MESSAGE(MSG("robot centre may travel x = %.1f .. %.1f cm along the line", fMin(), fMax()));
    TEST_MESSAGE(MSG("ARM_X_CM may be %.2f .. %.2f cm (now %.2f)", armLo, armHi, ARM_X_CM));
    TEST_MESSAGE(MSG("scan starts with the ultrasonic at x = %.1f cm: a cube in the left deposit "
                     "with its centre below that is not seen on the first pass",
                     fMin() + ULTRASONIC_X_CM));
}

// ---------- Body ----------

void test_width_within_limit() {
    TEST_ASSERT_TRUE_MESSAGE(geometry::widthFits(ROBOT_WIDTH_CM, MAX_WIDTH_CM),
        MSG("ROBOT_WIDTH_CM is %.2f cm, limit %.2f cm: make the robot %.2f cm narrower "
            "(wider robots touch deposit lines while following the colony line)",
            ROBOT_WIDTH_CM, MAX_WIDTH_CM, ROBOT_WIDTH_CM - MAX_WIDTH_CM));
}

void test_turning_radius_within_limit() {
    const float maxLen = geometry::maxHalfLength(ROBOT_WIDTH_CM, MAX_TURN_RADIUS_CM);
    TEST_ASSERT_TRUE_MESSAGE(turnR() <= MAX_TURN_RADIUS_CM,
        MSG("turning radius is %.2f cm, limit %.2f cm (180-degree turn between two deposits): "
            "at %.1f cm width, ROBOT_FRONT_CM and ROBOT_REAR_CM must each be <= %.2f cm",
            turnR(), MAX_TURN_RADIUS_CM, ROBOT_WIDTH_CM, maxLen));
}

void test_wheels_inside_body() {
    TEST_ASSERT_TRUE_MESSAGE(WHEEL_BASE_CM < ROBOT_WIDTH_CM,
        MSG("WHEEL_BASE_CM %.2f cm must be smaller than ROBOT_WIDTH_CM %.2f cm "
            "(the width includes the wheels)", WHEEL_BASE_CM, ROBOT_WIDTH_CM));
    const float r = WHEEL_DIAMETER_CM * 0.5f;
    TEST_ASSERT_TRUE_MESSAGE(r <= ROBOT_FRONT_CM && r <= ROBOT_REAR_CM,
        MSG("wheel radius %.2f cm sticks out past ROBOT_FRONT_CM %.2f or ROBOT_REAR_CM %.2f "
            "(front and rear include the wheels)", r, ROBOT_FRONT_CM, ROBOT_REAR_CM));
}

// ---------- Sensors ----------

void test_line_sensor_ahead_of_axle_on_centreline() {
    TEST_ASSERT_TRUE_MESSAGE(LINE_SENSOR_X_CM > 0.0f && LINE_SENSOR_X_CM <= ROBOT_FRONT_CM,
        MSG("LINE_SENSOR_X_CM is %.2f cm: it must be ahead of the axle (> 0) and inside the "
            "body (<= ROBOT_FRONT_CM %.2f) so the robot can steer onto the line",
            LINE_SENSOR_X_CM, ROBOT_FRONT_CM));
    TEST_ASSERT_TRUE_MESSAGE(LINE_SENSOR_Y_CM >= -LINE_SENSOR_MAX_OFFSET_Y_CM &&
                             LINE_SENSOR_Y_CM <= LINE_SENSOR_MAX_OFFSET_Y_CM,
        MSG("LINE_SENSOR_Y_CM is %.2f cm: the line sensor must be on the centreline (within %.2f cm)",
            LINE_SENSOR_Y_CM, LINE_SENSOR_MAX_OFFSET_Y_CM));
}

void test_ultrasonic_ahead_of_arm() {
    const float ahead = ULTRASONIC_X_CM - ARM_X_CM;
    TEST_ASSERT_TRUE_MESSAGE(ahead >= MIN_US_AHEAD_OF_ARM_CM,
        MSG("ultrasonic is %.2f cm ahead of the arm, needs >= %.2f cm: move it %.2f cm forward "
            "or the arm back (otherwise the robot must reverse after every scan)",
            ahead, MIN_US_AHEAD_OF_ARM_CM, MIN_US_AHEAD_OF_ARM_CM - ahead));
}

void test_ultrasonic_on_left_side_inside_body() {
    TEST_ASSERT_TRUE_MESSAGE(ULTRASONIC_Y_CM > 0.0f &&
        geometry::insideBody(ULTRASONIC_X_CM, ULTRASONIC_Y_CM, ROBOT_WIDTH_CM, ROBOT_FRONT_CM, ROBOT_REAR_CM),
        MSG("ultrasonic at x %.2f, y %.2f cm: it must be on the left side (y > 0) and inside the "
            "body (|y| <= %.2f, x from -%.2f to %.2f); if it sticks out, increase the body size",
            ULTRASONIC_X_CM, ULTRASONIC_Y_CM, ROBOT_WIDTH_CM * 0.5f, ROBOT_REAR_CM, ROBOT_FRONT_CM));
}

void test_imu_inside_body() {
    TEST_ASSERT_TRUE_MESSAGE(
        geometry::insideBody(IMU_X_CM, IMU_Y_CM, ROBOT_WIDTH_CM, ROBOT_FRONT_CM, ROBOT_REAR_CM),
        MSG("IMU at x %.2f, y %.2f cm is outside the body; mount it near the axle centre",
            IMU_X_CM, IMU_Y_CM));
}

// ---------- Arm ----------

void test_arm_inside_body_when_retracted() {
    TEST_ASSERT_TRUE_MESSAGE(ARM_GRIP_RETRACTED_Y_CM > 0.0f &&
                             ARM_GRIP_RETRACTED_Y_CM <= ROBOT_WIDTH_CM * 0.5f,
        MSG("ARM_GRIP_RETRACTED_Y_CM is %.2f cm: the retracted gripper must stay inside the "
            "width (<= %.2f cm from the centreline)", ARM_GRIP_RETRACTED_Y_CM, ROBOT_WIDTH_CM * 0.5f));
    TEST_ASSERT_TRUE_MESSAGE(ARM_X_CM >= -ROBOT_REAR_CM && ARM_X_CM <= ROBOT_FRONT_CM,
        MSG("ARM_X_CM is %.2f cm, outside the body (-%.2f .. %.2f)",
            ARM_X_CM, ROBOT_REAR_CM, ROBOT_FRONT_CM));
}

void test_arm_reaches_cube_in_deposit() {
    TEST_ASSERT_TRUE_MESSAGE(reach() >= needReach(),
        MSG("the grip point reaches %.2f cm beyond the side, needs %.2f cm to a cube in the "
            "middle of a deposit (%.1f cm from the line): add %.2f cm to ARM_STROKE_CM",
            reach(), needReach(), depositDist(), needReach() - reach()));
}

void test_arm_reaches_zone_centre() {
    TEST_ASSERT_TRUE_MESSAGE(reach() >= needZoneReach(),
        MSG("the grip point reaches %.2f cm beyond the side, needs %.2f cm to the centre of a "
            "zone (%.1f cm from the line): add %.2f cm to ARM_STROKE_CM",
            reach(), needZoneReach(), zoneDist(), needZoneReach() - reach()));
}

// ---------- Stations along the line (arm position vs the territory fence) ----------

void test_every_zone_reachable_inside_fence() {
    for (int z = 0; z < 5; ++z) {
        const float axle = geometry::axleXForTarget(ZONE_X[z], ARM_X_CM, false);  // after the 180
        TEST_ASSERT_TRUE_MESSAGE(geometry::inRange(axle, fMin(), fMax()),
            MSG("to place in the %s zone (x %.1f) the robot centre must stop at x %.2f, outside "
                "the fence %.2f .. %.2f: move the arm closer to the axle (ARM_X_CM %.2f) or "
                "reduce the turning radius", ZONE_NAME[z], ZONE_X[z], axle, fMin(), fMax(), ARM_X_CM));
    }
}

void test_every_deposit_cube_pickable_inside_fence() {
    for (int d = 0; d < 3; ++d) {
        const float ends[2] = {cubeMinX(d), cubeMaxX(d)};
        for (int i = 0; i < 2; ++i) {
            const float axle = geometry::axleXForTarget(ends[i], ARM_X_CM, true);  // facing +x
            TEST_ASSERT_TRUE_MESSAGE(geometry::inRange(axle, fMin(), fMax()),
                MSG("to pick a cube at x %.1f in the %s deposit the robot centre must stop at "
                    "x %.2f, outside the fence %.2f .. %.2f: move the arm closer to the axle "
                    "(ARM_X_CM %.2f) or reduce the turning radius",
                    ends[i], DEPOSIT_NAME[d], axle, fMin(), fMax(), ARM_X_CM));
        }
    }
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_print_design_report);
    RUN_TEST(test_width_within_limit);
    RUN_TEST(test_turning_radius_within_limit);
    RUN_TEST(test_wheels_inside_body);
    RUN_TEST(test_line_sensor_ahead_of_axle_on_centreline);
    RUN_TEST(test_ultrasonic_ahead_of_arm);
    RUN_TEST(test_ultrasonic_on_left_side_inside_body);
    RUN_TEST(test_imu_inside_body);
    RUN_TEST(test_arm_inside_body_when_retracted);
    RUN_TEST(test_arm_reaches_cube_in_deposit);
    RUN_TEST(test_arm_reaches_zone_centre);
    RUN_TEST(test_every_zone_reachable_inside_fence);
    RUN_TEST(test_every_deposit_cube_pickable_inside_fence);
    return UNITY_END();
}
