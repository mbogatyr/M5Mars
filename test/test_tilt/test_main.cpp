#include <math.h>
#include <unity.h>

#include "Tilt.h"

// An accelerometer reading for a pose, in degrees: `side` moves the reading
// along the long Y axis (one end of the stick lifted), `tip` turns it around
// that axis, between X and Z. The pilot's neutral grip is tipped by 45.
struct Reading {
    float x, y, z;
};

static constexpr float kGrip = 45.0f;

static Reading pose(float side, float tip = kGrip) {
    const float s = side / 57.2957795f;
    const float t = tip / 57.2957795f;
    return {cosf(s) * sinf(t), sinf(s), cosf(s) * cosf(t)};
}

static void recenter(Tilt &tilt, Reading r) { tilt.recenter(r.x, r.y, r.z); }

// Holds a pose long enough for the filter to settle.
static void hold(Tilt &tilt, Reading r) {
    for (int i = 0; i < 60; ++i) {
        tilt.update(r.x, r.y, r.z);
    }
}

static bool moved(Tilt &tilt, Reading r) { return tilt.moved(r.x, r.y, r.z); }

static Tilt centered() {
    Tilt tilt;
    recenter(tilt, pose(0));
    return tilt;
}

void setUp(void) {}
void tearDown(void) {}

void test_the_neutral_grip_gives_no_input(void) {
    Tilt tilt = centered();
    hold(tilt, pose(0));
    TEST_ASSERT_EQUAL_FLOAT(0, tilt.turn());
    TEST_ASSERT_EQUAL_FLOAT(0, tilt.climb());
}

void test_small_tilts_are_ignored(void) {
    Tilt tilt = centered();
    hold(tilt, pose(3, kGrip - 3));
    TEST_ASSERT_EQUAL_FLOAT(0, tilt.turn());
    TEST_ASSERT_EQUAL_FLOAT(0, tilt.climb());
}

void test_tilting_to_one_side_turns_one_way(void) {
    Tilt tilt = centered();
    hold(tilt, pose(17));
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.5f, tilt.turn()); // halfway from the dead zone to full
    hold(tilt, pose(-17));
    TEST_ASSERT_FLOAT_WITHIN(0.02f, -0.5f, tilt.turn());
}

void test_a_big_tilt_is_full_deflection(void) {
    Tilt tilt = centered();
    hold(tilt, pose(50));
    TEST_ASSERT_EQUAL_FLOAT(1, tilt.turn());
    hold(tilt, pose(0, kGrip - 60));
    TEST_ASSERT_EQUAL_FLOAT(-1, tilt.climb());
}

void test_tipping_climbs_one_way_and_dives_the_other(void) {
    Tilt tilt = centered();
    hold(tilt, pose(0, kGrip + 17));
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.5f, tilt.climb());
    TEST_ASSERT_EQUAL_FLOAT(0, tilt.turn());
    hold(tilt, pose(0, kGrip - 17));
    TEST_ASSERT_FLOAT_WITHIN(0.02f, -0.5f, tilt.climb());
}

// A reading near +-180 degrees of tip must not jump to the other extreme.
void test_tip_is_counted_across_the_half_turn(void) {
    Tilt tilt;
    recenter(tilt, pose(0, 170));
    hold(tilt, pose(0, -173)); // 17 degrees further on
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 0.5f, tilt.climb());
}

void test_recenter_makes_any_grip_neutral(void) {
    Tilt tilt = centered();
    hold(tilt, pose(20, 80));
    TEST_ASSERT_NOT_EQUAL(0, tilt.turn());
    recenter(tilt, pose(20, 80));
    TEST_ASSERT_EQUAL_FLOAT(0, tilt.turn());
    hold(tilt, pose(20, 80));
    TEST_ASSERT_EQUAL_FLOAT(0, tilt.turn());
    TEST_ASSERT_EQUAL_FLOAT(0, tilt.climb());
}

void test_the_first_reading_becomes_neutral(void) {
    Tilt tilt;
    hold(tilt, pose(25, 70));
    TEST_ASSERT_EQUAL_FLOAT(0, tilt.turn());
    TEST_ASSERT_EQUAL_FLOAT(0, tilt.climb());
}

void test_a_sudden_tilt_is_smoothed(void) {
    Tilt tilt = centered();
    const Reading r = pose(40);
    tilt.update(r.x, r.y, r.z);
    TEST_ASSERT_TRUE(tilt.turn() > 0 && tilt.turn() < 1);
}

void test_a_glitched_reading_is_ignored(void) {
    Tilt tilt = centered();
    hold(tilt, pose(17));
    const float before = tilt.turn();
    tilt.update(0, 0, 0);
    tilt.update(0.01f, 0, 0.02f);
    TEST_ASSERT_EQUAL_FLOAT(before, tilt.turn());
}

void test_moving_the_stick_counts_as_activity(void) {
    Tilt tilt;
    TEST_ASSERT_FALSE(moved(tilt, pose(0))); // the first reading only sets the anchor
    TEST_ASSERT_TRUE(moved(tilt, pose(10)));
    TEST_ASSERT_FALSE(moved(tilt, pose(10)));
}

void test_a_stick_lying_still_is_no_activity(void) {
    Tilt tilt;
    moved(tilt, pose(30, 10));
    for (int i = 0; i < 100; ++i) {
        const float jitter = (i % 3 - 1) * 0.5f; // sensor noise, degrees
        TEST_ASSERT_FALSE(moved(tilt, pose(30 + jitter, 10 - jitter)));
    }
}

// Small steps add up: the anchor stays where it was until it fires.
void test_a_slow_turn_adds_up_to_activity(void) {
    Tilt tilt;
    moved(tilt, pose(0));
    TEST_ASSERT_FALSE(moved(tilt, pose(5)));
    TEST_ASSERT_TRUE(moved(tilt, pose(10)));
}

int main(int, char **) {
    UNITY_BEGIN();

    RUN_TEST(test_the_neutral_grip_gives_no_input);
    RUN_TEST(test_small_tilts_are_ignored);
    RUN_TEST(test_tilting_to_one_side_turns_one_way);
    RUN_TEST(test_a_big_tilt_is_full_deflection);
    RUN_TEST(test_tipping_climbs_one_way_and_dives_the_other);
    RUN_TEST(test_tip_is_counted_across_the_half_turn);
    RUN_TEST(test_recenter_makes_any_grip_neutral);
    RUN_TEST(test_the_first_reading_becomes_neutral);
    RUN_TEST(test_a_sudden_tilt_is_smoothed);
    RUN_TEST(test_a_glitched_reading_is_ignored);
    RUN_TEST(test_moving_the_stick_counts_as_activity);
    RUN_TEST(test_a_stick_lying_still_is_no_activity);
    RUN_TEST(test_a_slow_turn_adds_up_to_activity);

    return UNITY_END();
}
