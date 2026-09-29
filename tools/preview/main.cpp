// Flies over a planet with the firmware's own lib/ code on the Mac and saves
// a contact sheet of frames, to check the look without the board.
//
//   c++ -std=gnu++17 -O2 -Ilib/Camera -Ilib/Terrain -Ilib/MarsPalette \
//       -Ilib/VoxelRenderer -Ilib/Flight tools/preview/main.cpp \
//       lib/Terrain/Terrain.cpp lib/MarsPalette/MarsPalette.cpp \
//       lib/VoxelRenderer/VoxelRenderer.cpp lib/Flight/Flight.cpp -lz -o /tmp/mars_preview
//   /tmp/mars_preview /tmp/sheet.png [seed]
//
// Six frames, 2x size, three to a row: cruising, turning right, turning left,
// climbing, diving, fast. It also prints the mean render time on the Mac
// (the board is much slower) for comparing changes.

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#include <chrono>
#include <vector>

#include "Flight.h"
#include "Terrain.h"
#include "VoxelRenderer.h"

namespace {

constexpr int kWidth = 240;
constexpr int kHeight = 135;
constexpr int kScale = 2;
constexpr int kColumns = 3;

void put32(std::vector<uint8_t> &v, uint32_t x) {
    for (int s = 24; s >= 0; s -= 8) {
        v.push_back(static_cast<uint8_t>(x >> s));
    }
}

void chunk(FILE *f, const char *kind, const std::vector<uint8_t> &data) {
    std::vector<uint8_t> out;
    put32(out, static_cast<uint32_t>(data.size()));
    std::vector<uint8_t> body(kind, kind + 4);
    body.insert(body.end(), data.begin(), data.end());
    out.insert(out.end(), body.begin(), body.end());
    put32(out, static_cast<uint32_t>(crc32(0, body.data(), static_cast<uInt>(body.size()))));
    fwrite(out.data(), 1, out.size(), f);
}

bool writePng(const char *path, int w, int h, const std::vector<uint8_t> &rgb) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        return false;
    }
    static const uint8_t sig[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    fwrite(sig, 1, 8, f);
    std::vector<uint8_t> ihdr;
    put32(ihdr, static_cast<uint32_t>(w));
    put32(ihdr, static_cast<uint32_t>(h));
    ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0});
    chunk(f, "IHDR", ihdr);
    std::vector<uint8_t> rows;
    for (int y = 0; y < h; ++y) {
        rows.push_back(0);
        rows.insert(rows.end(), rgb.begin() + y * w * 3, rgb.begin() + (y + 1) * w * 3);
    }
    uLongf size = compressBound(static_cast<uLong>(rows.size()));
    std::vector<uint8_t> z(size);
    compress2(z.data(), &size, rows.data(), static_cast<uLong>(rows.size()), 9);
    z.resize(size);
    chunk(f, "IDAT", z);
    chunk(f, "IEND", {});
    fclose(f);
    return true;
}

struct Shot {
    const char *name;
    Controls controls;
    int fastSpeed; // 1: switch to the fast level before this shot
};

Terrain terrain;
VoxelRenderer renderer;
uint16_t frame[kWidth * kHeight];

} // namespace

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s sheet.png [seed]\n", argv[0]);
        return 1;
    }
    const uint32_t seed = argc > 2 ? static_cast<uint32_t>(strtoul(argv[2], nullptr, 0)) : 1993;

    auto t0 = std::chrono::steady_clock::now();
    terrain.generate(seed);
    auto t1 = std::chrono::steady_clock::now();
    printf("generate: %.1f ms\n", std::chrono::duration<double, std::milli>(t1 - t0).count());

    const Shot shots[] = {
        {"cruise", {0, 0}, 0},  {"right", {1, 0}, 0},  {"left", {-1, 0}, 0},
        {"climb", {0, 1}, 0},   {"dive", {0, -1}, 0},  {"fast", {0, 0}, 1},
    };
    const int count = sizeof(shots) / sizeof(shots[0]);
    const int rowsOfShots = (count + kColumns - 1) / kColumns;
    const int sheetW = kColumns * kWidth * kScale;
    const int sheetH = rowsOfShots * kHeight * kScale;
    std::vector<uint8_t> sheet(sheetW * sheetH * 3, 0);

    Flight flight;
    uint32_t now = 0;
    flight.begin(now, terrain);
    double renderMs = 0;
    int renders = 0;

    for (int s = 0; s < count; ++s) {
        if (shots[s].fastSpeed) {
            flight.nextSpeed();
        }
        // Three seconds of flight with these controls, then the picture.
        for (int t = 0; t < 3000; t += 25) {
            now += 25;
            flight.update(now, shots[s].controls, terrain);
            auto r0 = std::chrono::steady_clock::now();
            renderer.render(terrain, flight.camera(), frame, kWidth, kHeight);
            renderMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - r0).count();
            ++renders;
        }
        const Camera &c = flight.camera();
        printf("%-6s x=%6.1f y=%6.1f heading=%5.2f alt=%5.1f ground=%5.1f pitch=%5.2f bank=%5.2f\n",
               shots[s].name, c.x, c.y, c.heading, c.altitude, terrain.groundAt(c.x, c.y), c.pitch,
               c.bank);

        const int ox = (s % kColumns) * kWidth * kScale;
        const int oy = (s / kColumns) * kHeight * kScale;
        for (int y = 0; y < kHeight * kScale; ++y) {
            for (int x = 0; x < kWidth * kScale; ++x) {
                const uint16_t v = frame[(y / kScale) * kWidth + x / kScale];
                const int r = (v >> 11) & 0x1F, g = (v >> 5) & 0x3F, b = v & 0x1F;
                uint8_t *p = &sheet[((oy + y) * sheetW + ox + x) * 3];
                p[0] = static_cast<uint8_t>(r << 3 | r >> 2);
                p[1] = static_cast<uint8_t>(g << 2 | g >> 4);
                p[2] = static_cast<uint8_t>(b << 3 | b >> 2);
            }
        }
    }
    printf("render: %.3f ms per frame on the Mac\n", renderMs / renders);
    if (!writePng(argv[1], sheetW, sheetH, sheet)) {
        fprintf(stderr, "cannot write %s\n", argv[1]);
        return 1;
    }
    printf("sheet -> %s\n", argv[1]);
    return 0;
}
