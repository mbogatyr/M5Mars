#pragma once

#include <math.h>

// Where the camera is and where it looks. Flight produces it, VoxelRenderer
// draws the view from it.
//
// The map lies in the x/y plane, with height going up. Heading turns
// counter-clockwise: forward is (cos, sin) and right is (sin, -cos), so
// turning right makes the heading smaller.
struct Camera {
    float x = 0;        // position on the map, in cells
    float y = 0;
    float heading = 0;  // radians
    float altitude = 0; // world units, like Terrain::groundAt
    float pitch = 0;    // tangent of the nose-up angle; below 0 looks down
    float bank = 0;     // tangent of the roll; above 0 the right wing is down

    float forwardX() const { return cosf(heading); }
    float forwardY() const { return sinf(heading); }
    float rightX() const { return sinf(heading); }
    float rightY() const { return -cosf(heading); }
};
