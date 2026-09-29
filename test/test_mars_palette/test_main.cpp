#include <stdlib.h>

#include <initializer_list>
#include <unity.h>

#include "MarsPalette.h"

using namespace MarsPalette;

static uint16_t fog[kFogLevels][256];
static uint16_t sky[kSkyLevels][kCloudLevels];

void setUp(void) {}
void tearDown(void) {}

static int clamp63(int v) { return v > 63 ? 63 : v; }

static int distance565(uint16_t a, uint16_t b) {
    return abs((a >> 11) - (b >> 11)) + abs(((a >> 5) & 0x3F) - ((b >> 5) & 0x3F)) +
           abs((a & 0x1F) - (b & 0x1F));
}

// At 6 bits per channel the ramp is exactly the VGA palette of mars.c.
void test_terrain_ramp_is_the_palette_of_mars_c(void) {
    for (int i = 0; i < 256; ++i) {
        const Rgb c = terrain(static_cast<uint8_t>(i));
        TEST_ASSERT_EQUAL(clamp63(i / 4 + 16), c.r >> 2);
        TEST_ASSERT_EQUAL(clamp63(i / 8), c.g >> 2);
        TEST_ASSERT_EQUAL(clamp63(i / 16), c.b >> 2);
    }
}

void test_ramp_uses_the_full_8_bit_range(void) {
    TEST_ASSERT_EQUAL(255, terrain(255).r);
    TEST_ASSERT_EQUAL(0, terrain(0).g);
}

void test_mix_goes_from_one_colour_to_the_other(void) {
    const Rgb from{10, 200, 30};
    const Rgb to{250, 0, 30};
    const Rgb start = mix(from, to, 0, 8);
    const Rgb end = mix(from, to, 8, 8);
    const Rgb half = mix(from, to, 4, 8);
    TEST_ASSERT_EQUAL(10, start.r);
    TEST_ASSERT_EQUAL(200, start.g);
    TEST_ASSERT_EQUAL(250, end.r);
    TEST_ASSERT_EQUAL(0, end.g);
    TEST_ASSERT_EQUAL(130, half.r);
    TEST_ASSERT_EQUAL(100, half.g);
    TEST_ASSERT_EQUAL(30, half.b);
}

void test_rgb565_packing(void) {
    TEST_ASSERT_EQUAL_HEX16(0xF800, toRgb565({255, 0, 0}));
    TEST_ASSERT_EQUAL_HEX16(0x07E0, toRgb565({0, 255, 0}));
    TEST_ASSERT_EQUAL_HEX16(0x001F, toRgb565({0, 0, 255}));
    TEST_ASSERT_EQUAL_HEX16(0x3412, swapBytes(0x1234));
}

void test_no_fog_leaves_the_colour_as_it_is(void) {
    buildFogLut(fog);
    for (int i = 0; i < 256; ++i) {
        TEST_ASSERT_EQUAL_HEX16(toRgb565(terrain(static_cast<uint8_t>(i))), fog[0][i]);
    }
}

void test_full_fog_is_pure_haze(void) {
    buildFogLut(fog);
    for (int i = 0; i < 256; ++i) {
        TEST_ASSERT_EQUAL_HEX16(toRgb565(haze()), fog[kFogLevels - 1][i]);
    }
}

void test_each_fog_level_is_closer_to_the_haze(void) {
    buildFogLut(fog);
    const uint16_t h = toRgb565(haze());
    for (int i : {0, 64, 128, 255}) {
        for (int level = 1; level < kFogLevels; ++level) {
            TEST_ASSERT_LESS_OR_EQUAL(distance565(fog[level - 1][i], h), distance565(fog[level][i], h));
        }
    }
}

// At the horizon the sky is the haze the far terrain fades into.
void test_sky_at_the_horizon_is_pure_haze_even_under_clouds(void) {
    buildSkyLut(sky);
    for (int d = 0; d < kCloudLevels; ++d) {
        TEST_ASSERT_EQUAL_HEX16(toRgb565(haze()), sky[kSkyLevels - 1][d]);
    }
}

void test_thin_cloud_leaves_the_sky_clear_and_thick_cloud_shows(void) {
    buildSkyLut(sky);
    TEST_ASSERT_EQUAL_HEX16(toRgb565(zenith()), sky[0][0]);
    TEST_ASSERT_EQUAL_HEX16(sky[0][0], sky[0][5]);
    TEST_ASSERT_GREATER_THAN(20, distance565(sky[0][0], sky[0][kCloudLevels - 1]));
}

int main(int, char **) {
    UNITY_BEGIN();

    RUN_TEST(test_terrain_ramp_is_the_palette_of_mars_c);
    RUN_TEST(test_ramp_uses_the_full_8_bit_range);
    RUN_TEST(test_mix_goes_from_one_colour_to_the_other);
    RUN_TEST(test_rgb565_packing);
    RUN_TEST(test_no_fog_leaves_the_colour_as_it_is);
    RUN_TEST(test_full_fog_is_pure_haze);
    RUN_TEST(test_each_fog_level_is_closer_to_the_haze);
    RUN_TEST(test_sky_at_the_horizon_is_pure_haze_even_under_clouds);
    RUN_TEST(test_thin_cloud_leaves_the_sky_clear_and_thick_cloud_shows);

    return UNITY_END();
}
