#pragma once

#include <stdint.h>

// Fills grid (2^sizeLog2 by 2^sizeLog2, row-major) with a Diamond-Square
// fractal, the algorithm MARS.EXE built its planet with.
//
// The grid is a torus: every average wraps around the edges, so the result
// tiles seamlessly and the flight can go on forever over a finite map.
// roughness (0..1) is how much the random displacement shrinks from one
// octave to the next: smaller values give rounder, smoother hills.
// The values are raw (they stay within int16) and still need normalizing.
void diamondSquare(int16_t *grid, int sizeLog2, uint32_t seed, float roughness);

// One pass of a [1 2 1] blur along rows and columns, wrapping around the
// edges like diamondSquare. Takes the needle-like peaks off the fractal.
void smoothTorus(int16_t *grid, int sizeLog2);

// The planet: a 256x256 height map with a colour per cell, and a smaller
// cloud texture for the sky. Plain data, no hardware.
//
// It takes 144 KB, so it is meant to be a static object: on the board that
// puts it in internal RAM, which is much faster than PSRAM for the random
// reads of the renderer.
class Terrain {
  public:
    static constexpr int kSizeLog2 = 8;
    static constexpr int kSize = 1 << kSizeLog2; // 256
    static constexpr int kMask = kSize - 1;

    static constexpr int kCloudLog2 = 7;
    static constexpr int kCloudSize = 1 << kCloudLog2; // 128
    static constexpr int kCloudMask = kCloudSize - 1;

    // World units per height step. The map is 256 cells wide; the highest
    // peak (255) stands about 75 cells tall.
    static constexpr float kHeightScale = 0.3f;

    // Builds a new planet. The same seed always gives the same planet.
    void generate(uint32_t seed);

    uint32_t seed() const { return seed_; }

    // Hand-made ground, for the tests: every cell the same, no clouds; then
    // single cells changed with set().
    void flatten(uint8_t height, uint8_t color);
    void set(int x, int y, uint8_t height, uint8_t color) {
        cells_[index(x, y)] = static_cast<uint16_t>(color << 8 | height);
    }

    // Coordinates wrap around, so any integers are valid.
    static int index(int x, int y) { return ((y & kMask) << kSizeLog2) | (x & kMask); }
    uint8_t height(int x, int y) const { return cells_[index(x, y)] & 0xFF; }
    uint8_t color(int x, int y) const { return cells_[index(x, y)] >> 8; }

    // Each cell is packed as color << 8 | height, so the renderer gets both
    // with a single read.
    const uint16_t *cells() const { return cells_; }

    // Cloud density, 0..255, kCloudSize by kCloudSize, wrapping.
    const uint8_t *clouds() const { return clouds_; }

    // Ground height in world units at any point of the map, interpolated
    // between the cells.
    float groundAt(float x, float y) const;

  private:
    void shade();

    uint16_t cells_[kSize * kSize];
    uint8_t clouds_[kCloudSize * kCloudSize];
    uint32_t seed_ = 0;
};
