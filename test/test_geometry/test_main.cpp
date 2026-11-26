// Geometry formula tests with plain numbers - run on the PC with:  pio test -e test
// (The check of the actual robot design against the limits is in test_robot_design.)
#include <unity.h>
#include "config.h"
#include "core/geometry.h"

void setUp() {}
void tearDown() {}

// ---------- Envelope formulas ----------

void test_max_half_length_for_16cm_width() {
    // sqrt(12.5^2 - 8^2) = 9.6 cm
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 9.6f, geometry::maxHalfLength(16.0f, 12.5f));
}

void test_turn_radius_uses_longer_side() {
    // half width 8, longer side 6 -> radius 10
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, geometry::turnRadius(16.0f, 6.0f, 3.0f));
}

void test_turn_clears_limits() {
    TEST_ASSERT_TRUE(geometry::turnClears(16.0f, 9.5f, 9.5f, 12.5f));
    TEST_ASSERT_FALSE(geometry::turnClears(16.0f, 10.0f, 8.0f, 12.5f));
}

void test_width_fits_limits() {
    TEST_ASSERT_TRUE(geometry::widthFits(16.0f, 16.0f));
    TEST_ASSERT_FALSE(geometry::widthFits(16.5f, 16.0f));
}

// Plan step 2, "done when": passes with W = 16 cm and R = 12.5 cm, fails with W = 20 cm.
void test_step2_16cm_robot_passes_20cm_robot_fails() {
    const float L = geometry::maxHalfLength(16.0f, 12.5f);   // longest legal length at 16 cm
    TEST_ASSERT_TRUE(geometry::widthFits(16.0f, cfg::MAX_WIDTH_CM));
    TEST_ASSERT_TRUE(geometry::turnClears(16.0f, L, L, cfg::MAX_TURN_RADIUS_CM));

    TEST_ASSERT_FALSE(geometry::widthFits(20.0f, cfg::MAX_WIDTH_CM));
    TEST_ASSERT_FALSE(geometry::turnClears(20.0f, L, L, cfg::MAX_TURN_RADIUS_CM));
}

// ---------- Arm, fence, stations ----------

void test_required_arm_reach() {
    // deposit cube centre 20 cm from the line, width 16 -> 20 - 8 = 12 cm beyond the side
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.0f, geometry::requiredArmReach(20.0f, 16.0f));
    // zone centre 17.5 cm from the line -> 9.5 cm
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 9.5f, geometry::requiredArmReach(17.5f, 16.0f));
}

void test_arm_reach_beyond_side() {
    // grip point at the side (8) + 15 stroke -> 15 beyond a 16 cm body; 1.5 cm inside -> 13.5
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.0f, geometry::armReachBeyondSide(8.0f, 15.0f, 16.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 13.5f, geometry::armReachBeyondSide(6.5f, 15.0f, 16.0f));
}

void test_field_distances_from_the_line() {
    // CLAUDE.md: deposit cube centre 20 cm from the colony line, zone centre 17.5 cm
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f,
        (cfg::DEPOSIT_Y_MIN + cfg::DEPOSIT_Y_MAX) * 0.5f - cfg::COLONY_LINE_Y);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 17.5f,
        cfg::COLONY_LINE_Y - (cfg::ZONE_Y_MIN + cfg::ZONE_Y_MAX) * 0.5f);
}

void test_fence_matches_plan_example() {
    // plan: "With R = 12 cm that is x = 15 to 115 cm"
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.0f, geometry::fenceMin(12.0f, 3.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 115.0f, geometry::fenceMax(130.0f, 12.0f, 3.0f));
}

void test_axle_x_for_target_both_directions() {
    // arm 2 cm ahead of the axle
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 48.0f, geometry::axleXForTarget(50.0f, 2.0f, true));   // facing +x
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 52.0f, geometry::axleXForTarget(50.0f, 2.0f, false));  // after 180
}

void test_inside_body() {
    TEST_ASSERT_TRUE(geometry::insideBody(9.0f, 8.0f, 16.0f, 9.0f, 9.0f));    // front-left corner
    TEST_ASSERT_TRUE(geometry::insideBody(-9.0f, -8.0f, 16.0f, 9.0f, 9.0f));  // rear-right corner
    TEST_ASSERT_FALSE(geometry::insideBody(9.1f, 0.0f, 16.0f, 9.0f, 9.0f));
    TEST_ASSERT_FALSE(geometry::insideBody(0.0f, 8.1f, 16.0f, 9.0f, 9.0f));
}

// ---------- The limits in config.h agree with the field layout ----------

void test_max_turn_radius_follows_from_field() {
    const float g = cfg::DEPOSIT_Y_MIN - cfg::COLONY_LINE_Y;            // 10 cm
    const float gapLeft  = cfg::DEPOSIT_X_MIN[1] - cfg::DEPOSIT_X_MAX[0];  // 15 cm
    const float gapRight = cfg::DEPOSIT_X_MIN[2] - cfg::DEPOSIT_X_MAX[1];  // 15 cm
    TEST_ASSERT_FLOAT_WITHIN(0.01f, cfg::MAX_TURN_RADIUS_CM, geometry::maxTurnRadiusInGap(g, gapLeft));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, cfg::MAX_TURN_RADIUS_CM, geometry::maxTurnRadiusInGap(g, gapRight));
}

void test_turn_spots_are_gap_centres() {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, (cfg::DEPOSIT_X_MAX[0] + cfg::DEPOSIT_X_MIN[1]) * 0.5f, cfg::TURN_SPOT_X[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, (cfg::DEPOSIT_X_MAX[1] + cfg::DEPOSIT_X_MIN[2]) * 0.5f, cfg::TURN_SPOT_X[1]);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_max_half_length_for_16cm_width);
    RUN_TEST(test_turn_radius_uses_longer_side);
    RUN_TEST(test_turn_clears_limits);
    RUN_TEST(test_width_fits_limits);
    RUN_TEST(test_step2_16cm_robot_passes_20cm_robot_fails);
    RUN_TEST(test_required_arm_reach);
    RUN_TEST(test_arm_reach_beyond_side);
    RUN_TEST(test_field_distances_from_the_line);
    RUN_TEST(test_fence_matches_plan_example);
    RUN_TEST(test_axle_x_for_target_both_directions);
    RUN_TEST(test_inside_body);
    RUN_TEST(test_max_turn_radius_follows_from_field);
    RUN_TEST(test_turn_spots_are_gap_centres);
    return UNITY_END();
}
