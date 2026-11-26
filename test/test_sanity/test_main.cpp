// Checks that the PC test setup itself works.
#include <unity.h>

void setUp() {}
void tearDown() {}

void test_unity_runs() {
    TEST_ASSERT_EQUAL_INT(2, 1 + 1);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_unity_runs);
    return UNITY_END();
}
