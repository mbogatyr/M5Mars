#pragma once

#include <stdint.h>

// The colours of the planet.
//
// The terrain ramp is the palette of mars.c, VGA with 6 bits per channel
// (r = i/4 + 16, g = i/8, b = i/16), widened to 8 bits. The sky and the
// distance colours were measured on a capture of the original MARS.EXE: a
// salmon-red sky with pink clouds that grows lighter towards the horizon, a
// bright line of glow on the horizon, and far ground that sinks into a dark
// maroon rather than into a light haze.
//
// Colours come out as plain RGB565; swapping the bytes for a particular
// display is the caller's business.
namespace MarsPalette {

struct Rgb {
    uint8_t r, g, b;
};

constexpr int kFogLevels = 16;   // 0: the colour as it is, last: farGround()
constexpr int kSkyLevels = 16;   // 0: high in the sky, last: at the horizon
constexpr int kCloudLevels = 16; // cloud texture density, 0: clear sky

Rgb terrain(uint8_t index);
Rgb farGround();   // what the ground fades to at the view distance
Rgb skyTop();      // clear sky overhead
Rgb skyHorizon();  // clear sky at the horizon
Rgb cloud();
Rgb horizonGlow(); // the bright line where the sky meets the far ground

// t/tMax of the way from a to b, rounded.
Rgb mix(Rgb a, Rgb b, int t, int tMax);

uint16_t toRgb565(Rgb c);
uint16_t swapBytes(uint16_t v);

// lut[level][index]: terrain colour `index` seen at distance `level`.
void buildFogLut(uint16_t lut[kFogLevels][256]);

// lut[level][density]: the sky at distance `level` with that much cloud.
void buildSkyLut(uint16_t lut[kSkyLevels][kCloudLevels]);

} // namespace MarsPalette
