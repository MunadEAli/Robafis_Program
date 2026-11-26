// Envelope geometry tests - run on the PC with:  pio test -e test
#include <unity.h>
#include "config.h"
#include "core/geometry.h"

void setUp() {}
void tearDown() {}

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

void test_current_config_is_legal() {
    TEST_ASSERT_TRUE(geometry::widthFits(cfg::ROBOT_WIDTH_CM, cfg::MAX_WIDTH_CM));
    TEST_ASSERT_TRUE(geometry::turnClears(cfg::ROBOT_WIDTH_CM, cfg::ROBOT_FRONT_CM,
                                          cfg::ROBOT_REAR_CM, cfg::MAX_TURN_RADIUS_CM));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_max_half_length_for_16cm_width);
    RUN_TEST(test_turn_radius_uses_longer_side);
    RUN_TEST(test_turn_clears_limits);
    RUN_TEST(test_width_fits_limits);
    RUN_TEST(test_current_config_is_legal);
    return UNITY_END();
}
