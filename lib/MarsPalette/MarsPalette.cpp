#include "MarsPalette.h"

namespace MarsPalette {

namespace {

// Cloud texture densities below this are clear sky.
constexpr int kCloudFrom = 7;
// How opaque the thickest cloud is overhead, in 1/256.
constexpr int kCloudOpacity = 220;

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

Rgb haze() { return {200, 128, 96}; }
Rgb zenith() { return {88, 36, 28}; }
Rgb cloud() { return {232, 176, 140}; }

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
                toRgb565(mix(terrain(static_cast<uint8_t>(i)), haze(), level, kFogLevels - 1));
        }
    }
}

void buildSkyLut(uint16_t lut[kSkyLevels][kCloudLevels]) {
    const int far = kSkyLevels - 1;
    for (int level = 0; level < kSkyLevels; ++level) {
        const Rgb sky = mix(zenith(), haze(), level, far);
        for (int density = 0; density < kCloudLevels; ++density) {
            const int cover = density > kCloudFrom ? density - kCloudFrom : 0;
            const int coverMax = kCloudLevels - 1 - kCloudFrom;
            // Clouds thin out with distance and vanish into the haze at the
            // horizon, so the sky meets the far terrain without a seam.
            const int alpha = kCloudOpacity * cover * (far - level) / (coverMax * far);
            lut[level][density] = toRgb565(mix(sky, cloud(), alpha, 256));
        }
    }
}

} // namespace MarsPalette
