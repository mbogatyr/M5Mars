#include "VoxelRenderer.h"

#include <math.h>

namespace {

constexpr float kFocalPerWidth = 0.6f; // about 80 degrees across

// Slices start this close and get thicker with distance: near detail
// matters, far away one cell is less than a pixel anyway.
constexpr float kNear = 2.0f;
constexpr float kFirstStep = 0.6f;
constexpr float kStepGrowth = 0.012f;

// The haze begins here and swallows everything at kFar.
constexpr float kFogStart = 30.0f;

// The cloud ceiling: how high above the camera it is, how many texels one
// map cell covers, and at what distance it is lost in the haze.
constexpr float kCloudAbove = 70.0f;
constexpr float kCloudTexelsPerCell = 0.25f;
constexpr float kSkyHazeDistance = 3200.0f;

// Keeps texture coordinates positive before they are truncated to int.
constexpr float kPositive = 65536.0f;

uint32_t toFixed(float v) { return static_cast<uint32_t>(static_cast<int32_t>(v * 65536.0f)); }

int fogLevel(float z) {
    constexpr int kLast = MarsPalette::kFogLevels - 1;
    if (z <= kFogStart) {
        return 0;
    }
    const int level = static_cast<int>((z - kFogStart) * kLast / (VoxelRenderer::kFar - kFogStart) + 0.5f);
    return level < kLast ? level : kLast;
}

} // namespace

VoxelRenderer::VoxelRenderer(bool byteSwapped) {
    MarsPalette::buildFogLut(fog_);
    MarsPalette::buildSkyLut(sky_);
    if (byteSwapped) {
        for (auto &level : fog_) {
            for (auto &c : level) {
                c = MarsPalette::swapBytes(c);
            }
        }
        for (auto &level : sky_) {
            for (auto &c : level) {
                c = MarsPalette::swapBytes(c);
            }
        }
    }
}

float VoxelRenderer::focal(int width) { return width * kFocalPerWidth; }

void VoxelRenderer::render(const Terrain &terrain, const Camera &camera, uint16_t *frame, int width,
                           int height) {
    if (width <= 0 || height <= 0 || width > kMaxWidth || height > kMaxHeight) {
        return;
    }
    const float f = focal(width);
    const float centre = width * 0.5f;
    const float horizon = height * 0.5f + camera.pitch * f;
    for (int i = 0; i < width; ++i) {
        // Banking right lifts the horizon on the right: the view shears the
        // way it would roll.
        horizon_[i] = horizon - (i + 0.5f - centre) * camera.bank;
        top_[i] = static_cast<int16_t>(height);
    }

    renderTerrain(terrain, camera, frame, width);
    renderSky(terrain, camera, frame, width);
}

void VoxelRenderer::renderTerrain(const Terrain &terrain, const Camera &camera, uint16_t *frame,
                                  int width) {
    const uint16_t *cells = terrain.cells();
    const float f = focal(width);
    const float fx = camera.forwardX();
    const float fy = camera.forwardY();
    const float rx = camera.rightX();
    const float ry = camera.rightY();
    // Sideways offset of column 0 per unit of depth.
    const float firstColumn = (0.5f - width * 0.5f) / f;

    int open = width; // columns not yet painted up to the top of the frame
    float z = kNear;
    float dz = kFirstStep;
    while (z < kFar && open > 0) {
        // The slice at depth z, from its left end, one step per column.
        const float side = firstColumn * z;
        uint32_t px = toFixed(camera.x + fx * z + rx * side);
        uint32_t py = toFixed(camera.y + fy * z + ry * side);
        const uint32_t dx = toFixed(rx * z / f);
        const uint32_t dy = toFixed(ry * z / f);

        // Screen row = horizon + (altitude - ground) * f / z.
        const float k = f / z;
        const float lift = camera.altitude * k;
        const float perStep = Terrain::kHeightScale * k;
        const uint16_t *lut = fog_[fogLevel(z)];

        for (int i = 0; i < width; ++i, px += dx, py += dy) {
            const int top = top_[i];
            if (top <= 0) {
                continue;
            }
            const int cellX = (px >> 16) & Terrain::kMask;
            const int cellY = (py >> 16) & Terrain::kMask;
            const uint16_t cell = cells[cellY << Terrain::kSizeLog2 | cellX];
            const float row = horizon_[i] + lift - (cell & 0xFF) * perStep;
            if (row >= top) {
                continue; // hidden behind nearer ground
            }
            const int y = row > 0 ? static_cast<int>(row) : 0;
            const uint16_t color = lut[cell >> 8];
            uint16_t *p = frame + y * width + i;
            for (int n = top - y; n > 0; --n, p += width) {
                *p = color;
            }
            top_[i] = static_cast<int16_t>(y);
            if (y == 0) {
                --open;
            }
        }

        z += dz;
        dz += kStepGrowth;
    }
}

void VoxelRenderer::prepareSky(int width) {
    if (skyWidth_ == width) {
        return;
    }
    skyWidth_ = width;
    const float f = focal(width);
    constexpr int kLast = MarsPalette::kSkyLevels - 1;
    for (int d = 0; d < kDistances; ++d) {
        // d rows above the horizon the ceiling is this far ahead.
        const float distance = kCloudAbove * f / (d > 0 ? d : 0.5f);
        cloudDistance_[d] = distance;
        const int level = static_cast<int>(distance * kLast / kSkyHazeDistance);
        skyLevel_[d] = static_cast<uint8_t>(level < kLast ? level : kLast);
    }
}

void VoxelRenderer::renderSky(const Terrain &terrain, const Camera &camera, uint16_t *frame,
                              int width) {
    prepareSky(width);
    const uint8_t *clouds = terrain.clouds();
    const float f = focal(width);
    const float centre = width * 0.5f;
    const float fx = camera.forwardX();
    const float fy = camera.forwardY();
    const float rx = camera.rightX();
    const float ry = camera.rightY();
    const float u0 = camera.x * kCloudTexelsPerCell + kPositive;
    const float v0 = camera.y * kCloudTexelsPerCell + kPositive;
    const uint16_t haze = hazeColor();

    for (int i = 0; i < width; ++i) {
        const int top = top_[i];
        if (top <= 0) {
            continue;
        }
        // Direction of this column's rays across the map, per unit ahead.
        const float side = (i + 0.5f - centre) / f;
        const float du = (fx + rx * side) * kCloudTexelsPerCell;
        const float dv = (fy + ry * side) * kCloudTexelsPerCell;
        const int horizon = static_cast<int>(floorf(horizon_[i]));

        uint16_t *p = frame + i;
        for (int y = 0; y < top; ++y, p += width) {
            int d = horizon - y;
            if (d <= 0) {
                *p = haze; // below the horizon, beyond the view distance
                continue;
            }
            if (d >= kDistances) {
                d = kDistances - 1;
            }
            const float distance = cloudDistance_[d];
            const int u = static_cast<int>(u0 + du * distance) & Terrain::kCloudMask;
            const int v = static_cast<int>(v0 + dv * distance) & Terrain::kCloudMask;
            const uint8_t density = clouds[v << Terrain::kCloudLog2 | u];
            *p = sky_[skyLevel_[d]][density >> 4];
        }
    }
}
