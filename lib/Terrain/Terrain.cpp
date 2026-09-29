#include "Terrain.h"

#include <math.h>

namespace {

// Displacement of the first, coarsest octave. With roughness below 0.6 the
// sum over all octaves stays well inside int16.
constexpr float kFirstAmplitude = 12000.0f;

constexpr float kRoughness = 0.46f;      // terrain: large hills, little grit
constexpr float kCloudRoughness = 0.62f; // clouds: more ragged edges

// Heights are raised to this power after normalizing: low ground flattens
// into plains and valleys, the peaks get steeper.
constexpr float kHeightCurve = 1.5f;

// Colour index = base + height share + slope shading + grain, then clamped.
// The indices go through MarsPalette's ramp, dark red at 0, orange at 255.
constexpr int kColorBase = 40;
constexpr int kColorPerHeight = 150; // added at the highest peak
constexpr int kShadePerSlope = 4;
constexpr int kGrain = 4; // like the rand() % 8 of mars.c

// xorshift32: small, fast and the same on the Mac and on the board.
class Random {
  public:
    explicit Random(uint32_t seed) : state_(seed * 2654435761u ^ 0x9E3779B9u) {
        if (state_ == 0) {
            state_ = 1;
        }
    }

    uint32_t next() {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 17;
        state_ ^= state_ << 5;
        return state_;
    }

    // Uniform in -amp..amp.
    int32_t spread(int32_t amp) {
        if (amp <= 0) {
            return 0;
        }
        return static_cast<int32_t>(next() % static_cast<uint32_t>(2 * amp + 1)) - amp;
    }

  private:
    uint32_t state_;
};

int16_t clamp16(int32_t v) {
    if (v > 32767) {
        return 32767;
    }
    if (v < -32767) {
        return -32767;
    }
    return static_cast<int16_t>(v);
}

int clampByte(int v) {
    if (v < 0) {
        return 0;
    }
    if (v > 255) {
        return 255;
    }
    return v;
}

// Maps the raw values linearly onto 0..255 and then through curve.
// in and out may be the same memory: each value is read before its own
// slot is written.
template <typename Out>
void normalize(const int16_t *in, int count, Out *out, const uint8_t *curve) {
    int16_t lo = in[0];
    int16_t hi = in[0];
    for (int i = 1; i < count; ++i) {
        if (in[i] < lo) {
            lo = in[i];
        }
        if (in[i] > hi) {
            hi = in[i];
        }
    }
    const int32_t range = hi > lo ? hi - lo : 1;
    for (int i = 0; i < count; ++i) {
        const int32_t v = (static_cast<int32_t>(in[i]) - lo) * 255 / range;
        out[i] = curve ? curve[v] : static_cast<uint8_t>(v);
    }
}

} // namespace

void diamondSquare(int16_t *grid, int sizeLog2, uint32_t seed, float roughness) {
    const int n = 1 << sizeLog2;
    const int mask = n - 1;
    auto at = [grid, sizeLog2, mask](int x, int y) -> int16_t & {
        return grid[((y & mask) << sizeLog2) | (x & mask)];
    };

    Random random(seed);
    at(0, 0) = 0;
    float amplitude = kFirstAmplitude;

    for (int step = n; step > 1; step /= 2) {
        const int half = step / 2;
        const int32_t a = static_cast<int32_t>(amplitude);

        // Diamond step: the centre of every square is the mean of its four
        // corners plus a random displacement.
        for (int y = 0; y < n; y += step) {
            for (int x = 0; x < n; x += step) {
                const int32_t mean =
                    (at(x, y) + at(x + step, y) + at(x, y + step) + at(x + step, y + step)) / 4;
                at(x + half, y + half) = clamp16(mean + random.spread(a));
            }
        }

        // Square step: the middle of every edge is the mean of its four
        // neighbours, two corners and two centres.
        for (int y = 0; y < n; y += half) {
            const int first = (y / half) % 2 == 0 ? half : 0;
            for (int x = first; x < n; x += step) {
                const int32_t mean =
                    (at(x - half, y) + at(x + half, y) + at(x, y - half) + at(x, y + half)) / 4;
                at(x, y) = clamp16(mean + random.spread(a));
            }
        }

        amplitude *= roughness;
    }
}

void smoothTorus(int16_t *grid, int sizeLog2) {
    constexpr int kMaxSize = 256;
    const int n = 1 << sizeLog2;
    const int mask = n - 1;
    if (n > kMaxSize) {
        return;
    }
    int16_t line[kMaxSize];

    for (int y = 0; y < n; ++y) {
        int16_t *row = grid + (y << sizeLog2);
        for (int x = 0; x < n; ++x) {
            line[x] = row[x];
        }
        for (int x = 0; x < n; ++x) {
            row[x] = static_cast<int16_t>(
                (line[(x - 1) & mask] + 2 * line[x] + line[(x + 1) & mask]) / 4);
        }
    }

    for (int x = 0; x < n; ++x) {
        for (int y = 0; y < n; ++y) {
            line[y] = grid[(y << sizeLog2) | x];
        }
        for (int y = 0; y < n; ++y) {
            grid[(y << sizeLog2) | x] = static_cast<int16_t>(
                (line[(y - 1) & mask] + 2 * line[y] + line[(y + 1) & mask]) / 4);
        }
    }
}

void Terrain::generate(uint32_t seed) {
    seed_ = seed;

    // cells_ doubles as the scratch grid: first for the clouds, then for
    // the heights, which are normalized in place.
    int16_t *scratch = reinterpret_cast<int16_t *>(cells_);

    diamondSquare(scratch, kCloudLog2, seed ^ 0xC10D5u, kCloudRoughness);
    normalize(scratch, kCloudSize * kCloudSize, clouds_, nullptr);

    uint8_t curve[256];
    for (int i = 0; i < 256; ++i) {
        curve[i] = static_cast<uint8_t>(lroundf(255.0f * powf(i / 255.0f, kHeightCurve)));
    }
    diamondSquare(scratch, kSizeLog2, seed, kRoughness);
    smoothTorus(scratch, kSizeLog2);
    normalize(scratch, kSize * kSize, cells_, curve);

    shade();
}

void Terrain::flatten(uint8_t height, uint8_t color) {
    for (auto &cell : cells_) {
        cell = static_cast<uint16_t>(color << 8 | height);
    }
    for (auto &c : clouds_) {
        c = 0;
    }
}

void Terrain::shade() {
    Random grain(seed_ ^ 0x6A11u);
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            const int h = height(x, y);
            // The sun is on the (-x, -y) side: slopes turned towards it are
            // lit, the ones turned away are in shadow. Only the low byte is
            // read here, so writing the colour as we go does no harm.
            const int slope = height(x + 1, y + 1) - height(x - 1, y - 1);
            const int c = kColorBase + h * kColorPerHeight / 255 + slope * kShadePerSlope +
                          grain.spread(kGrain);
            cells_[index(x, y)] = static_cast<uint16_t>(clampByte(c) << 8 | h);
        }
    }
}

float Terrain::groundAt(float x, float y) const {
    const float fx = floorf(x);
    const float fy = floorf(y);
    const int ix = static_cast<int>(fx);
    const int iy = static_cast<int>(fy);
    const float tx = x - fx;
    const float ty = y - fy;

    const float h00 = height(ix, iy);
    const float h10 = height(ix + 1, iy);
    const float h01 = height(ix, iy + 1);
    const float h11 = height(ix + 1, iy + 1);
    const float top = h00 + (h10 - h00) * tx;
    const float bottom = h01 + (h11 - h01) * tx;
    return (top + (bottom - top) * ty) * kHeightScale;
}
