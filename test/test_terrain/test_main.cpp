#include <stdlib.h>
#include <string.h>
#include <unity.h>

#include "Terrain.h"

// Terrain takes 144 KB: static, not on the stack.
static Terrain a;
static Terrain b;

void setUp(void) {}
void tearDown(void) {}

void test_same_seed_gives_the_same_planet(void) {
    a.generate(7);
    b.generate(7);
    TEST_ASSERT_EQUAL_MEMORY(a.cells(), b.cells(), sizeof(uint16_t) * Terrain::kSize * Terrain::kSize);
    TEST_ASSERT_EQUAL_MEMORY(a.clouds(), b.clouds(), Terrain::kCloudSize * Terrain::kCloudSize);
}

void test_different_seeds_give_different_planets(void) {
    a.generate(7);
    b.generate(8);
    int differ = 0;
    for (int i = 0; i < Terrain::kSize * Terrain::kSize; ++i) {
        differ += a.cells()[i] != b.cells()[i];
    }
    TEST_ASSERT_GREATER_THAN(Terrain::kSize * Terrain::kSize / 2, differ);
}

void test_heights_span_the_whole_range(void) {
    a.generate(1993);
    int lo = 255, hi = 0;
    for (int y = 0; y < Terrain::kSize; ++y) {
        for (int x = 0; x < Terrain::kSize; ++x) {
            const int h = a.height(x, y);
            lo = h < lo ? h : lo;
            hi = h > hi ? h : hi;
        }
    }
    TEST_ASSERT_EQUAL(0, lo);
    TEST_ASSERT_EQUAL(255, hi);
}

void test_clouds_span_the_whole_range(void) {
    a.generate(1993);
    int lo = 255, hi = 0;
    for (int i = 0; i < Terrain::kCloudSize * Terrain::kCloudSize; ++i) {
        const int c = a.clouds()[i];
        lo = c < lo ? c : lo;
        hi = c > hi ? c : hi;
    }
    TEST_ASSERT_EQUAL(0, lo);
    TEST_ASSERT_EQUAL(255, hi);
}

// Across the seam of the torus neighbours differ no more than inside.
void test_map_tiles_seamlessly(void) {
    a.generate(42);
    const int last = Terrain::kSize - 1;
    const int mid = Terrain::kSize / 2;
    long seamX = 0, seamY = 0, insideX = 0, insideY = 0;
    for (int i = 0; i < Terrain::kSize; ++i) {
        seamX += abs(a.height(last, i) - a.height(0, i));
        insideX += abs(a.height(mid - 1, i) - a.height(mid, i));
        seamY += abs(a.height(i, last) - a.height(i, 0));
        insideY += abs(a.height(i, mid - 1) - a.height(i, mid));
    }
    TEST_ASSERT_LESS_THAN(2 * insideX + Terrain::kSize, seamX);
    TEST_ASSERT_LESS_THAN(2 * insideY + Terrain::kSize, seamY);
}

void test_diamond_square_fills_every_cell(void) {
    const int log2 = 4;
    int16_t grid[1 << (2 * log2)];
    for (auto &v : grid) {
        v = -32768; // a value diamondSquare never produces
    }
    diamondSquare(grid, log2, 5, 0.5f);
    for (auto v : grid) {
        TEST_ASSERT_NOT_EQUAL(-32768, v);
    }
}

void test_smoothing_keeps_a_flat_grid_flat(void) {
    const int log2 = 3;
    int16_t grid[1 << (2 * log2)];
    for (auto &v : grid) {
        v = 1000;
    }
    smoothTorus(grid, log2);
    for (auto v : grid) {
        TEST_ASSERT_EQUAL(1000, v);
    }
}

// Slopes turned towards the sun on the -x, -y side are lit, the ones turned
// away are dark: compared at the same range of heights.
void test_slopes_facing_the_sun_are_brighter(void) {
    a.generate(1993);
    long litSum = 0, darkSum = 0;
    int lit = 0, dark = 0;
    for (int y = 0; y < Terrain::kSize; ++y) {
        for (int x = 0; x < Terrain::kSize; ++x) {
            const int h = a.height(x, y);
            if (h < 60 || h > 180) {
                continue;
            }
            const int slope = a.height(x + 1, y + 1) - a.height(x - 1, y - 1);
            if (slope > 6) {
                litSum += a.color(x, y);
                ++lit;
            } else if (slope < -6) {
                darkSum += a.color(x, y);
                ++dark;
            }
        }
    }
    TEST_ASSERT_GREATER_THAN(100, lit);
    TEST_ASSERT_GREATER_THAN(100, dark);
    TEST_ASSERT_GREATER_THAN(darkSum / dark + 30, litSum / lit);
}

void test_ground_is_interpolated_between_cells(void) {
    a.flatten(0, 0);
    a.set(10, 20, 100, 0);
    a.set(11, 20, 200, 0);
    TEST_ASSERT_EQUAL_FLOAT(100 * Terrain::kHeightScale, a.groundAt(10, 20));
    TEST_ASSERT_EQUAL_FLOAT(150 * Terrain::kHeightScale, a.groundAt(10.5f, 20));
    TEST_ASSERT_EQUAL_FLOAT(75 * Terrain::kHeightScale, a.groundAt(10.5f, 20.5f));
}

void test_ground_wraps_around_the_map(void) {
    a.generate(3);
    TEST_ASSERT_EQUAL_FLOAT(a.groundAt(10.25f, 99.5f), a.groundAt(10.25f + Terrain::kSize, 99.5f));
    TEST_ASSERT_EQUAL_FLOAT(a.groundAt(10.25f, 99.5f), a.groundAt(10.25f, 99.5f - Terrain::kSize));
}

int main(int, char **) {
    UNITY_BEGIN();

    RUN_TEST(test_same_seed_gives_the_same_planet);
    RUN_TEST(test_different_seeds_give_different_planets);
    RUN_TEST(test_heights_span_the_whole_range);
    RUN_TEST(test_clouds_span_the_whole_range);
    RUN_TEST(test_map_tiles_seamlessly);
    RUN_TEST(test_diamond_square_fills_every_cell);
    RUN_TEST(test_smoothing_keeps_a_flat_grid_flat);
    RUN_TEST(test_slopes_facing_the_sun_are_brighter);
    RUN_TEST(test_ground_is_interpolated_between_cells);
    RUN_TEST(test_ground_wraps_around_the_map);

    return UNITY_END();
}
