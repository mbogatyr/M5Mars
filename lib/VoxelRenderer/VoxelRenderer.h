#pragma once

#include <stdint.h>

#include "Camera.h"
#include "MarsPalette.h"
#include "Terrain.h"

// Draws the view over the planet the way MARS.EXE (and later Comanche) did.
//
// The ground is walked from near to far in slices of growing depth. Each
// slice is a line across the map, sampled once per screen column; the height
// found there is projected onto the screen and the column is filled up to it,
// but only above what nearer slices have already painted. That keeps every
// pixel painted once and hides what is behind the hills without a z-buffer.
// Far slices fade into the haze. What is left above the ground is sky: a
// ceiling of fractal clouds seen in perspective.
//
// It writes into any RGB565 buffer and never touches hardware, so it runs in
// the tests on the Mac as well.
class VoxelRenderer {
  public:
    static constexpr int kMaxWidth = 320;
    static constexpr int kMaxHeight = 240;

    static constexpr float kFar = 320.0f; // view distance, in cells

    // byteSwapped: write RGB565 with the high byte first, the way
    // LovyanGFX sprites keep their pixels.
    explicit VoxelRenderer(bool byteSwapped = false);

    // Paints the whole frame: width x height pixels, row-major. Sizes above
    // kMaxWidth x kMaxHeight are not drawn at all.
    void render(const Terrain &terrain, const Camera &camera, uint16_t *frame, int width,
                int height);

    // Focal length in pixels for a frame of this width.
    static float focal(int width);

    // The colours it paints with, for the tests.
    uint16_t fogColor(int level, uint8_t index) const { return fog_[level][index]; }
    uint16_t hazeColor() const { return sky_[MarsPalette::kSkyLevels - 1][0]; }

  private:
    static constexpr int kDistances = 512;

    void prepareSky(int width);
    void renderTerrain(const Terrain &terrain, const Camera &camera, uint16_t *frame, int width);
    void renderSky(const Terrain &terrain, const Camera &camera, uint16_t *frame, int width);

    uint16_t fog_[MarsPalette::kFogLevels][256];
    uint16_t sky_[MarsPalette::kSkyLevels][MarsPalette::kCloudLevels];

    int16_t top_[kMaxWidth];   // per column: the highest row painted so far
    float horizon_[kMaxWidth]; // per column: the horizon row, tilted by the bank

    // By rows above the horizon: how far away the cloud ceiling is there,
    // and how much haze covers it. They depend only on the focal length.
    float cloudDistance_[kDistances];
    uint8_t skyLevel_[kDistances];
    int skyWidth_ = 0;
};
