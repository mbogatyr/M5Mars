#pragma once

#include <M5Unified.h>

#include "Camera.h"
#include "Terrain.h"
#include "VoxelRenderer.h"

// Shows the flight on the built-in StickS3 display, in landscape (240x135),
// the closest the stick gets to the 320x200 of VGA mode 13h.
//
// VoxelRenderer paints the whole frame straight into a sprite, which then
// goes out in a single push: drawing directly on the screen flickers. Every
// frame is new, since the camera never stops.
class Renderer {
  public:
    // Call after M5.begin().
    void begin();

    // caption: a short line shown over the top of the view, or nullptr.
    void draw(const Terrain &terrain, const Camera &camera, const char *caption);

    // The last frame over serial: "SNAP <w> <h>\n", then RGB565, high byte first.
    void writeSnapshot(Print &out);

    uint32_t paintUs() const { return paintUs_; }
    uint32_t pushUs() const { return pushUs_; }

  private:
    M5Canvas canvas_{&M5.Display};
    // The sprite keeps RGB565 with its bytes swapped, ready for SPI.
    VoxelRenderer voxel_{true};

    uint32_t paintUs_ = 0;
    uint32_t pushUs_ = 0;
};
