#include <math.h>
#include <unity.h>

#include "Flight.h"

static Terrain flat;
static Terrain planet;

void setUp(void) { flat.flatten(0, 100); }
void tearDown(void) {}

// Distance on the torus, so a wrap across the edge does not count as a leap.
static float travelled(const Camera &from, const Camera &to) {
    auto delta = [](float a, float b) {
        float d = fabsf(b - a);
        return d > Terrain::kSize / 2 ? Terrain::kSize - d : d;
    };
    return hypotf(delta(from.x, to.x), delta(from.y, to.y));
}

static float angleBetween(float from, float to) {
    float d = fmodf(to - from + 3 * 3.14159265f, 2 * 3.14159265f);
    return d - 3.14159265f;
}

// Flies `ms` in 25 ms steps (40 fps), starting at `startMs`; returns the end time.
static uint32_t fly(Flight &flight, const Terrain &terrain, uint32_t startMs, uint32_t ms,
                    Controls controls = {}) {
    uint32_t now = startMs;
    for (uint32_t t = 0; t < ms; t += 25) {
        now += 25;
        flight.update(now, controls, terrain);
    }
    return now;
}

void test_takes_off_at_cruising_height(void) {
    Flight flight;
    flight.begin(0, flat);
    TEST_ASSERT_EQUAL_FLOAT(Flight::kClearance, flight.camera().altitude);
}

void test_flies_at_its_speed(void) {
    Flight flight;
    flight.begin(0, flat);
    const Camera start = flight.camera();
    fly(flight, flat, 0, 1000);
    TEST_ASSERT_FLOAT_WITHIN(0.3f, flight.speed(), travelled(start, flight.camera()));
}

void test_a_long_gap_is_flown_as_one_short_step(void) {
    Flight flight;
    flight.begin(0, flat);
    const Camera start = flight.camera();
    flight.update(5000, {}, flat); // the display slept for 5 s
    TEST_ASSERT_FLOAT_WITHIN(0.01f, flight.speed() * Flight::kMaxStepMs / 1000.0f,
                             travelled(start, flight.camera()));
}

void test_survives_the_millis_rollover(void) {
    Flight flight;
    const uint32_t nearOverflow = 0xFFFFFFF0u;
    flight.begin(nearOverflow, flat);
    const Camera start = flight.camera();
    flight.update(nearOverflow + 40, {}, flat); // wrapped past zero
    TEST_ASSERT_FLOAT_WITHIN(0.01f, flight.speed() * 0.04f, travelled(start, flight.camera()));
}

void test_the_same_time_twice_changes_nothing(void) {
    Flight flight;
    flight.begin(0, flat);
    flight.update(100, {}, flat);
    const Camera before = flight.camera();
    flight.update(100, {}, flat);
    TEST_ASSERT_EQUAL_FLOAT(before.x, flight.camera().x);
    TEST_ASSERT_EQUAL_FLOAT(before.y, flight.camera().y);
}

void test_position_stays_on_the_map(void) {
    Flight flight;
    flight.nextSpeed(); // fast
    flight.begin(0, flat);
    uint32_t now = 0;
    for (int i = 0; i < 60; ++i) {
        now = fly(flight, flat, now, 500);
        TEST_ASSERT_TRUE(flight.camera().x >= 0 && flight.camera().x < Terrain::kSize);
        TEST_ASSERT_TRUE(flight.camera().y >= 0 && flight.camera().y < Terrain::kSize);
    }
}

// Right turns make the heading smaller (Camera.h), and the view banks.
void test_turning_right_turns_right_of_the_autopilot_and_banks(void) {
    Flight straight, turning;
    straight.begin(0, flat);
    turning.begin(0, flat);
    fly(straight, flat, 0, 1000);
    fly(turning, flat, 0, 1000, {1, 0});
    TEST_ASSERT_LESS_THAN_FLOAT(-0.5f, angleBetween(straight.camera().heading, turning.camera().heading));
    TEST_ASSERT_GREATER_THAN_FLOAT(0.1f, turning.camera().bank);
}

void test_turning_left_turns_left_of_the_autopilot_and_banks(void) {
    Flight straight, turning;
    straight.begin(0, flat);
    turning.begin(0, flat);
    fly(straight, flat, 0, 1000);
    fly(turning, flat, 0, 1000, {-1, 0});
    TEST_ASSERT_GREATER_THAN_FLOAT(0.5f, angleBetween(straight.camera().heading, turning.camera().heading));
    TEST_ASSERT_LESS_THAN_FLOAT(-0.1f, turning.camera().bank);
}

void test_the_autopilot_wanders_by_itself(void) {
    Flight flight;
    flight.begin(0, flat);
    const float start = flight.camera().heading;
    fly(flight, flat, 0, 10000);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.3f, fabsf(angleBetween(start, flight.camera().heading)));
}

void test_climbing_goes_up_to_the_climb_range(void) {
    Flight cruising, climbing;
    cruising.begin(0, flat);
    climbing.begin(0, flat);
    fly(cruising, flat, 0, 5000);
    fly(climbing, flat, 0, 5000, {0, 1});
    TEST_ASSERT_FLOAT_WITHIN(0.5f, Flight::kClearance + Flight::kClimbRange, climbing.camera().altitude);
    TEST_ASSERT_GREATER_THAN_FLOAT(cruising.camera().pitch, climbing.camera().pitch); // nose up
}

void test_diving_goes_down_to_just_above_the_ground(void) {
    Flight flight;
    flight.begin(0, flat);
    fly(flight, flat, 0, 15000, {0, -1});
    TEST_ASSERT_FLOAT_WITHIN(0.5f, Flight::kMinClearance, flight.camera().altitude);
}

void test_letting_go_returns_to_cruising_height(void) {
    Flight flight;
    flight.begin(0, flat);
    uint32_t now = fly(flight, flat, 0, 5000, {0, 1});
    fly(flight, flat, now, 15000);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, Flight::kClearance, flight.camera().altitude);
}

// Over real mountains, diving at full speed, the camera never goes into the
// ground.
void test_never_below_the_ground(void) {
    planet.generate(1993);
    Flight flight;
    flight.nextSpeed(); // fast
    flight.begin(0, planet);
    uint32_t now = 0;
    for (int i = 0; i < 2400; ++i) {
        now += 25;
        flight.update(now, {i % 200 < 100 ? 1.0f : -1.0f, -1}, planet);
        const Camera &c = flight.camera();
        TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(planet.groundAt(c.x, c.y) + Flight::kMinClearance - 0.001f,
                                     c.altitude);
    }
}

void test_speed_levels_cycle(void) {
    Flight flight;
    TEST_ASSERT_EQUAL(1, flight.speedLevel());
    const float cruise = flight.speed();
    flight.nextSpeed();
    TEST_ASSERT_EQUAL(2, flight.speedLevel());
    TEST_ASSERT_GREATER_THAN_FLOAT(cruise, flight.speed());
    flight.nextSpeed();
    TEST_ASSERT_EQUAL(0, flight.speedLevel());
    TEST_ASSERT_LESS_THAN_FLOAT(cruise, flight.speed());
    flight.nextSpeed();
    TEST_ASSERT_EQUAL(1, flight.speedLevel());
}

void test_combined_controls_add_up_and_stay_in_range(void) {
    Controls tilt;
    tilt.turn = 0.3f;
    tilt.climb = -0.8f;
    Controls keys;
    keys.turn = 1;
    keys.climb = -1;
    const Controls both = combine(tilt, keys);
    TEST_ASSERT_EQUAL_FLOAT(1, both.turn);
    TEST_ASSERT_EQUAL_FLOAT(-1, both.climb);
    const Controls tiltOnly = combine(tilt, Controls{});
    TEST_ASSERT_EQUAL_FLOAT(0.3f, tiltOnly.turn);
    TEST_ASSERT_EQUAL_FLOAT(-0.8f, tiltOnly.climb);
}

int main(int, char **) {
    UNITY_BEGIN();

    RUN_TEST(test_takes_off_at_cruising_height);
    RUN_TEST(test_flies_at_its_speed);
    RUN_TEST(test_a_long_gap_is_flown_as_one_short_step);
    RUN_TEST(test_survives_the_millis_rollover);
    RUN_TEST(test_the_same_time_twice_changes_nothing);
    RUN_TEST(test_position_stays_on_the_map);
    RUN_TEST(test_turning_right_turns_right_of_the_autopilot_and_banks);
    RUN_TEST(test_turning_left_turns_left_of_the_autopilot_and_banks);
    RUN_TEST(test_the_autopilot_wanders_by_itself);
    RUN_TEST(test_climbing_goes_up_to_the_climb_range);
    RUN_TEST(test_diving_goes_down_to_just_above_the_ground);
    RUN_TEST(test_letting_go_returns_to_cruising_height);
    RUN_TEST(test_never_below_the_ground);
    RUN_TEST(test_speed_levels_cycle);
    RUN_TEST(test_combined_controls_add_up_and_stay_in_range);

    return UNITY_END();
}
