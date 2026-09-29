#pragma once

#include <stdint.h>

// The colours of the planet.
//
// The terrain ramp is the palette of mars.c, VGA with 6 bits per channel
// (r = i/4 + 16, g = i/8, b = i/16), widened to 8 bits. The haze, the sky
// and the clouds are picked to go with it.
//
// Colours come out as plain RGB565; swapping the bytes for a particular
// display is the caller's business.
namespace MarsPalette {

struct Rgb {
    uint8_t r, g, b;
};

constexpr int kFogLevels = 16;   // 0: the colour as it is, last: pure haze
constexpr int kSkyLevels = 16;   // 0: high in the sky, last: the horizon (pure haze)
constexpr int kCloudLevels = 16; // cloud texture density, 0: clear sky

Rgb terrain(uint8_t index);
Rgb haze();
Rgb zenith();
Rgb cloud();

// t/tMax of the way from a to b, rounded.
Rgb mix(Rgb a, Rgb b, int t, int tMax);

uint16_t toRgb565(Rgb c);
uint16_t swapBytes(uint16_t v);

// lut[level][index]: terrain colour `index` seen through fog `level`.
void buildFogLut(uint16_t lut[kFogLevels][256]);

// lut[level][density]: the sky at distance `level` with that much cloud.
void buildSkyLut(uint16_t lut[kSkyLevels][kCloudLevels]);

} // namespace MarsPalette
