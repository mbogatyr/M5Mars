#include <unity.h>

#include "DisplayTimeout.h"

// By default the display turns off after 180000 ms without button presses.
static DisplayTimeout startedTimeout() {
    DisplayTimeout timeout;
    timeout.begin(0);
    return timeout;
}

void setUp(void) {}
void tearDown(void) {}

void test_display_is_on_right_after_start(void) {
    DisplayTimeout timeout = startedTimeout();
    TEST_ASSERT_TRUE(timeout.shouldBeOn(0, false));
}

void test_display_stays_on_until_the_timeout_elapses(void) {
    DisplayTimeout timeout = startedTimeout();
    TEST_ASSERT_TRUE(timeout.shouldBeOn(179999, false));
}

void test_display_turns_off_when_the_timeout_elapses(void) {
    DisplayTimeout timeout = startedTimeout();
    TEST_ASSERT_FALSE(timeout.shouldBeOn(180000, false));
}

void test_display_stays_off_while_nothing_happens(void) {
    DisplayTimeout timeout = startedTimeout();
    timeout.shouldBeOn(180000, false);
    TEST_ASSERT_FALSE(timeout.shouldBeOn(500000, false));
}

void test_button_activity_wakes_a_sleeping_display(void) {
    DisplayTimeout timeout = startedTimeout();
    timeout.shouldBeOn(180000, false);
    TEST_ASSERT_TRUE(timeout.shouldBeOn(180001, true));
}

void test_button_activity_restarts_the_countdown(void) {
    DisplayTimeout timeout = startedTimeout();
    timeout.shouldBeOn(100000, true); // activity moves the starting point
    TEST_ASSERT_TRUE(timeout.shouldBeOn(279999, false));
    TEST_ASSERT_FALSE(timeout.shouldBeOn(280000, false));
}

void test_custom_timeout_is_respected(void) {
    DisplayTimeout timeout(1000);
    timeout.begin(0);
    TEST_ASSERT_TRUE(timeout.shouldBeOn(999, false));
    TEST_ASSERT_FALSE(timeout.shouldBeOn(1000, false));
}

void test_timeout_survives_the_millis_rollover(void) {
    DisplayTimeout timeout;
    const uint32_t nearOverflow = 0xFFFFFF00u; // 256 ms before the rollover
    timeout.begin(nearOverflow);
    // 100 ms later the counter has already wrapped past zero
    TEST_ASSERT_TRUE(timeout.shouldBeOn(nearOverflow + 100, false));
    TEST_ASSERT_FALSE(timeout.shouldBeOn(nearOverflow + 180000, false));
}

int main(int, char **) {
    UNITY_BEGIN();

    RUN_TEST(test_display_is_on_right_after_start);
    RUN_TEST(test_display_stays_on_until_the_timeout_elapses);
    RUN_TEST(test_display_turns_off_when_the_timeout_elapses);
    RUN_TEST(test_display_stays_off_while_nothing_happens);
    RUN_TEST(test_button_activity_wakes_a_sleeping_display);
    RUN_TEST(test_button_activity_restarts_the_countdown);
    RUN_TEST(test_custom_timeout_is_respected);
    RUN_TEST(test_timeout_survives_the_millis_rollover);

    return UNITY_END();
}
