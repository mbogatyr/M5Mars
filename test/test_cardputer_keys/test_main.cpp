#include <unity.h>

#include "CardputerKeys.h"

// A TCA8418 key event byte for matrix row r, column c.
static uint8_t press(int row, int column) { return static_cast<uint8_t>(0x80 | (1 + row * 10 + column)); }
static uint8_t release(int row, int column) { return static_cast<uint8_t>(1 + row * 10 + column); }

static bool same(KeyPos a, KeyPos b) { return a.x == b.x && a.y == b.y; }

void setUp(void) {}
void tearDown(void) {}

void test_the_first_key_is_the_top_left_one(void) {
    const KeyEvent e = decodeTca8418(press(0, 0));
    TEST_ASSERT_TRUE(e.valid);
    TEST_ASSERT_TRUE(e.pressed);
    TEST_ASSERT_TRUE(same({0, 0}, e.pos)); // `
}

// Matrix row 6 holds the last two printed columns: Backspace, Enter, Space...
void test_space_and_enter_are_in_the_last_column(void) {
    TEST_ASSERT_TRUE(same(CardputerKey::kSpace, decodeTca8418(press(6, 7)).pos));
    TEST_ASSERT_TRUE(same(CardputerKey::kEnter, decodeTca8418(press(6, 6)).pos));
}

void test_the_arrow_keys(void) {
    TEST_ASSERT_TRUE(same(CardputerKey::kUp, decodeTca8418(press(5, 6)).pos));    // ;
    TEST_ASSERT_TRUE(same(CardputerKey::kLeft, decodeTca8418(press(5, 3)).pos));  // ,
    TEST_ASSERT_TRUE(same(CardputerKey::kDown, decodeTca8418(press(5, 7)).pos));  // .
    TEST_ASSERT_TRUE(same(CardputerKey::kRight, decodeTca8418(press(6, 3)).pos)); // /
}

void test_bit_7_tells_a_press_from_a_release(void) {
    TEST_ASSERT_TRUE(decodeTca8418(press(2, 1)).pressed);
    const KeyEvent e = decodeTca8418(release(2, 1));
    TEST_ASSERT_TRUE(e.valid);
    TEST_ASSERT_FALSE(e.pressed);
}

void test_no_event_and_gpio_events_are_ignored(void) {
    TEST_ASSERT_FALSE(decodeTca8418(0x00).valid);
    TEST_ASSERT_FALSE(decodeTca8418(0x80).valid);
    TEST_ASSERT_FALSE(decodeTca8418(0x5B).valid); // first GPIO event
    TEST_ASSERT_FALSE(decodeTca8418(press(0, 8)).valid); // columns 8 and 9 are not wired
    TEST_ASSERT_FALSE(decodeTca8418(press(7, 0)).valid); // nor is row 7
}

void test_a_key_is_held_from_its_press_to_its_release(void) {
    KeyGrid grid;
    TEST_ASSERT_FALSE(grid.anyHeld());
    grid.apply(decodeTca8418(press(6, 7)));
    TEST_ASSERT_TRUE(grid.held(CardputerKey::kSpace));
    TEST_ASSERT_TRUE(grid.anyHeld());
    grid.apply(decodeTca8418(press(5, 3)));
    grid.apply(decodeTca8418(release(6, 7)));
    TEST_ASSERT_FALSE(grid.held(CardputerKey::kSpace));
    TEST_ASSERT_TRUE(grid.held(CardputerKey::kLeft));
    grid.clear();
    TEST_ASSERT_FALSE(grid.anyHeld());
}

void test_invalid_events_leave_the_grid_alone(void) {
    KeyGrid grid;
    grid.apply(decodeTca8418(0x00));
    grid.apply(decodeTca8418(0xDB));
    TEST_ASSERT_FALSE(grid.anyHeld());
}

int main(int, char **) {
    UNITY_BEGIN();

    RUN_TEST(test_the_first_key_is_the_top_left_one);
    RUN_TEST(test_space_and_enter_are_in_the_last_column);
    RUN_TEST(test_the_arrow_keys);
    RUN_TEST(test_bit_7_tells_a_press_from_a_release);
    RUN_TEST(test_no_event_and_gpio_events_are_ignored);
    RUN_TEST(test_a_key_is_held_from_its_press_to_its_release);
    RUN_TEST(test_invalid_events_leave_the_grid_alone);

    return UNITY_END();
}
