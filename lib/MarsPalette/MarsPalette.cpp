#include "MarsPalette.h"

namespace MarsPalette {

namespace {

// Cloud texture densities up to this are clear sky; above it the pink
// builds up smoothly, so the clouds are soft streaks rather than blobs.
constexpr int kCloudFrom = 4;
// How opaque the thickest cloud is, in 1/256. Unlike a haze it does not
// thin out with distance: in the original the streaks run right down to the
// horizon.
constexpr int kCloudOpacity = 230;

uint8_t widen6(int v) {
    if (v > 63) {
        v = 63;
    }
    return static_cast<uint8_t>(v << 2 | v >> 4);
}

} // namespace

Rgb terrain(uint8_t index) {
    return {widen6(index / 4 + 16), widen6(index / 8), widen6(index / 16)};
}

Rgb farGround() { return {48, 4, 0}; }
Rgb skyTop() { return {214, 62, 48}; }
Rgb skyHorizon() { return {222, 118, 104}; }
Rgb cloud() { return {228, 160, 150}; }
Rgb horizonGlow() { return {242, 132, 104}; }

Rgb mix(Rgb a, Rgb b, int t, int tMax) {
    auto channel = [t, tMax](int from, int to) {
        return static_cast<uint8_t>(from + ((to - from) * t * 2 + (to > from ? tMax : -tMax)) /
                                               (2 * tMax));
    };
    return {channel(a.r, b.r), channel(a.g, b.g), channel(a.b, b.b)};
}

uint16_t toRgb565(Rgb c) {
    return static_cast<uint16_t>((c.r >> 3) << 11 | (c.g >> 2) << 5 | (c.b >> 3));
}

uint16_t swapBytes(uint16_t v) { return static_cast<uint16_t>(v >> 8 | v << 8); }

void buildFogLut(uint16_t lut[kFogLevels][256]) {
    for (int level = 0; level < kFogLevels; ++level) {
        for (int i = 0; i < 256; ++i) {
            lut[level][i] =
                toRgb565(mix(terrain(static_cast<uint8_t>(i)), farGround(), level, kFogLevels - 1));
        }
    }
}

void buildSkyLut(uint16_t lut[kSkyLevels][kCloudLevels]) {
    const int far = kSkyLevels - 1;
    const int coverMax = kCloudLevels - 1 - kCloudFrom;
    for (int level = 0; level < kSkyLevels; ++level) {
        const Rgb sky = mix(skyTop(), skyHorizon(), level, far);
        for (int density = 0; density < kCloudLevels; ++density) {
            const int cover = density > kCloudFrom ? density - kCloudFrom : 0;
            lut[level][density] = toRgb565(mix(sky, cloud(), kCloudOpacity * cover / coverMax, 256));
        }
    }
}

} // namespace MarsPalette
