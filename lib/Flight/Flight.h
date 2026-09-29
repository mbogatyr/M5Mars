#pragma once

#include <stdint.h>

#include "Camera.h"
#include "Terrain.h"

// What the pilot asks for, each from -1 to 1.
struct Controls {
    float turn = 0;  // above 0 turns right
    float climb = 0; // above 0 climbs, below 0 dives down to the ground
};

// Flies the camera over the planet.
//
// The autopilot is always on: the course wanders by itself and the altitude
// follows the ground ahead. The controls bend the course and the height the
// autopilot keeps, so letting go simply hands the flight back to it.
//
// Like everything in lib/, it does not read the clock: the time comes in as
// an argument.
class Flight {
  public:
    static constexpr int kSpeedLevels = 3;

    // A longer gap between updates (the display slept, for instance) is
    // flown as this much, so the camera never jumps.
    static constexpr uint32_t kMaxStepMs = 100;

    static constexpr float kClearance = 18.0f;   // cruising height above the ground ahead
    static constexpr float kMinClearance = 3.0f; // the camera never goes lower
    static constexpr float kClimbRange = 40.0f;  // extra height at full climb

    // Starts in the middle of the map.
    Flight() { camera_.x = camera_.y = Terrain::kSize / 2; }

    // Takes off over this terrain from the current position, at cruising
    // height. Call at start-up and after the planet changes.
    void begin(uint32_t nowMs, const Terrain &terrain);

    void update(uint32_t nowMs, const Controls &controls, const Terrain &terrain);

    // Slow, cruise, fast, then slow again.
    void nextSpeed();
    int speedLevel() const { return speedLevel_; }
    float speed() const; // cells per second

    const Camera &camera() const { return camera_; }

  private:
    float groundAhead(const Terrain &terrain) const;

    Camera camera_;
    uint32_t lastMs_ = 0;
    float clock_ = 0;    // seconds flown, drives the autopilot
    float turnRate_ = 0; // radians per second, above 0 to the right
    int speedLevel_ = 1;
};
