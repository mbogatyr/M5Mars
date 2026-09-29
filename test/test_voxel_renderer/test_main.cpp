#include <math.h>
#include <unity.h>

#include "VoxelRenderer.h"

// A small frame: the focal length is 0.6 * 40 = 24 pixels and with no pitch
// the horizon lies on row 15.
static constexpr int kW = 40;
static constexpr int kH = 30;
static constexpr int kHorizon = kH / 2;

static constexpr uint8_t kGround = 100; // colour indices
static constexpr uint8_t kNearWall = 20;
static constexpr uint8_t kFarWall = 240;
static constexpr uint8_t kPillar = 180;

static Terrain terrain;
static VoxelRenderer renderer;
static uint16_t frame[kW * kH];

void setUp(void) { terrain.flatten(0, kGround); }
void tearDown(void) {}

static uint16_t pixel(int x, int y) { return frame[y * kW + x]; }

// Whether the pixel is terrain colour `index` seen through any fog level.
static bool isColor(uint16_t p, uint8_t index) {
    for (int level = 0; level < MarsPalette::kFogLevels - 1; ++level) {
        if (p == renderer.fogColor(level, index)) {
            return true;
        }
    }
    return false;
}

static int fogLevelOf(uint16_t p, uint8_t index) {
    for (int level = 0; level < MarsPalette::kFogLevels; ++level) {
        if (p == renderer.fogColor(level, index)) {
            return level;
        }
    }
    return -1;
}

// The topmost row of the column painted with the ground colour, or kH.
static int firstGroundRow(int x) {
    for (int y = 0; y < kH; ++y) {
        if (isColor(pixel(x, y), kGround)) {
            return y;
        }
    }
    return kH;
}

static Camera level(float altitude) {
    Camera c;
    c.x = 100;
    c.y = 128;
    c.heading = 0; // looking along +x
    c.altitude = altitude;
    return c;
}

// A wall across the whole map, `depth` cells thick, starting at x.
static void wall(int x, int depth, uint8_t height, uint8_t color) {
    for (int dx = 0; dx < depth; ++dx) {
        for (int y = 0; y < Terrain::kSize; ++y) {
            terrain.set(x + dx, y, height, color);
        }
    }
}

void test_flat_ground_is_below_the_horizon_and_sky_above(void) {
    renderer.render(terrain, level(20), frame, kW, kH);
    for (int x = 0; x < kW; ++x) {
        for (int y = 0; y < kHorizon; ++y) {
            TEST_ASSERT_FALSE_MESSAGE(isColor(pixel(x, y), kGround), "ground above the horizon");
        }
        for (int y = kHorizon + 2; y < kH; ++y) {
            TEST_ASSERT_TRUE_MESSAGE(isColor(pixel(x, y), kGround), "no ground below the horizon");
        }
    }
}

void test_clear_sky_is_the_same_across_a_row(void) {
    renderer.render(terrain, level(20), frame, kW, kH);
    for (int x = 1; x < kW; ++x) {
        TEST_ASSERT_EQUAL_HEX16(pixel(0, 2), pixel(x, 2));
    }
}

void test_far_ground_sinks_further_into_the_distance_colour(void) {
    renderer.render(terrain, level(20), frame, kW, kH);
    const int nearLevel = fogLevelOf(pixel(kW / 2, kH - 1), kGround);
    const int farLevel = fogLevelOf(pixel(kW / 2, kHorizon + 2), kGround);
    TEST_ASSERT_EQUAL(0, nearLevel);
    TEST_ASSERT_GREATER_THAN(3, farLevel);
}

void test_the_horizon_line_glows(void) {
    renderer.render(terrain, level(20), frame, kW, kH);
    for (int x = 0; x < kW; ++x) {
        TEST_ASSERT_EQUAL_HEX16(renderer.glowColor(), pixel(x, kHorizon));
    }
}

// Seen from high up, the ground ends below the horizon; the rows in between
// are the dark far colour, as if the ground went on.
void test_beyond_the_view_distance_lies_dark_ground(void) {
    renderer.render(terrain, level(100), frame, kW, kH); // the far edge is on row 20
    TEST_ASSERT_EQUAL_HEX16(renderer.farColor(), pixel(kW / 2, kHorizon + 3));
}

// The near wall is lower than the eye, the far one higher: the far wall
// shows above the near one, the near one hides the rest of it and all the
// ground in between.
void test_nearer_ground_hides_what_is_behind_it(void) {
    wall(110, 3, 70, kNearWall); // 21 units high, 10 cells ahead
    wall(140, 3, 100, kFarWall); // 30 units high, 40 cells ahead
    renderer.render(terrain, level(20), frame, kW, kH);
    const int x = kW / 2;
    // Row of a height seen from 20 units up: 15 + (20 - height) * 24 / distance.
    TEST_ASSERT_FALSE(isColor(pixel(x, 5), kFarWall)); // sky over both
    TEST_ASSERT_FALSE(isColor(pixel(x, 5), kNearWall));
    TEST_ASSERT_TRUE(isColor(pixel(x, 10), kFarWall));  // rows 9..11
    TEST_ASSERT_TRUE(isColor(pixel(x, 13), kNearWall)); // from row 12 down
    for (int y = 12; y < kH; ++y) {
        TEST_ASSERT_TRUE(isColor(pixel(x, y), kNearWall));
    }
}

void test_pitching_up_lowers_the_horizon(void) {
    Camera camera = level(20);
    renderer.render(terrain, camera, frame, kW, kH);
    const int straight = firstGroundRow(kW / 2);
    camera.pitch = 0.2f; // about 5 rows at this focal length
    renderer.render(terrain, camera, frame, kW, kH);
    TEST_ASSERT_GREATER_OR_EQUAL(straight + 4, firstGroundRow(kW / 2));
}

// Banked to the right, the horizon rises on the right side of the frame.
void test_banking_right_lifts_the_horizon_on_the_right(void) {
    Camera camera = level(20);
    camera.bank = 0.2f;
    renderer.render(terrain, camera, frame, kW, kH);
    TEST_ASSERT_LESS_THAN(firstGroundRow(2) - 4, firstGroundRow(kW - 3));
}

static void pillar(int x, int y) {
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            terrain.set(x + dx, y + dy, 255, kPillar);
        }
    }
}

// Only the near pillar, with no fog on it: the map wraps around, so far
// copies of it can show up elsewhere in the view.
static bool pillarInColumns(int from, int to) {
    for (int x = from; x < to; ++x) {
        for (int y = 0; y < kH; ++y) {
            if (pixel(x, y) == renderer.fogColor(0, kPillar)) {
                return true;
            }
        }
    }
    return false;
}

// Right of the camera is -y at heading 0 (see Camera.h). A smaller heading
// turns right and brings the pillar to the middle.
void test_turning_towards_a_pillar_brings_it_to_the_middle(void) {
    pillar(130, 118); // 30 ahead, 10 to the right
    Camera camera = level(20);
    renderer.render(terrain, camera, frame, kW, kH);
    TEST_ASSERT_TRUE(pillarInColumns(kW / 2 + 2, kW));
    TEST_ASSERT_FALSE(pillarInColumns(0, kW / 2));

    camera.heading = -atan2f(10, 30);
    renderer.render(terrain, camera, frame, kW, kH);
    TEST_ASSERT_TRUE(pillarInColumns(kW / 2 - 1, kW / 2 + 1));
}

void test_the_view_wraps_around_the_map(void) {
    Camera camera = level(20);
    terrain.generate(11);
    renderer.render(terrain, camera, frame, kW, kH);
    static uint16_t first[kW * kH];
    for (int i = 0; i < kW * kH; ++i) {
        first[i] = frame[i];
    }
    camera.x += Terrain::kSize;
    camera.y -= Terrain::kSize;
    renderer.render(terrain, camera, frame, kW, kH);
    // The same view, up to float rounding right at the edge of a cell.
    int differ = 0;
    for (int i = 0; i < kW * kH; ++i) {
        differ += first[i] != frame[i];
    }
    TEST_ASSERT_LESS_OR_EQUAL(kW * kH / 100, differ);
}

void test_byte_swapped_renderer_writes_swapped_pixels(void) {
    static VoxelRenderer swapped(true);
    static uint16_t other[kW * kH];
    renderer.render(terrain, level(20), frame, kW, kH);
    swapped.render(terrain, level(20), other, kW, kH);
    for (int i = 0; i < kW * kH; ++i) {
        TEST_ASSERT_EQUAL_HEX16(MarsPalette::swapBytes(frame[i]), other[i]);
    }
}

int main(int, char **) {
    UNITY_BEGIN();

    RUN_TEST(test_flat_ground_is_below_the_horizon_and_sky_above);
    RUN_TEST(test_clear_sky_is_the_same_across_a_row);
    RUN_TEST(test_far_ground_sinks_further_into_the_distance_colour);
    RUN_TEST(test_the_horizon_line_glows);
    RUN_TEST(test_beyond_the_view_distance_lies_dark_ground);
    RUN_TEST(test_nearer_ground_hides_what_is_behind_it);
    RUN_TEST(test_pitching_up_lowers_the_horizon);
    RUN_TEST(test_banking_right_lifts_the_horizon_on_the_right);
    RUN_TEST(test_turning_towards_a_pillar_brings_it_to_the_middle);
    RUN_TEST(test_the_view_wraps_around_the_map);
    RUN_TEST(test_byte_swapped_renderer_writes_swapped_pixels);

    return UNITY_END();
}
