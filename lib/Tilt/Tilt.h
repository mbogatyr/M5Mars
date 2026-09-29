#pragma once

#include <stdint.h>

// Turns the tilt of the stick, read by its accelerometer, into steering.
//
// The stick is held in landscape with the screen towards the pilot, like a
// tiny gamepad. Tilting it left or right (lifting one end) turns; tipping the
// top edge away dives and towards climbs, like a control column. Angles are
// counted from the neutral pose taken by recenter(), so any comfortable grip
// works.
//
// Board axes, as M5Unified reports them for the StickS3: X runs across the
// short side of the board, Y along the long side, Z out of the screen.
//
// Like everything in lib/, it never touches hardware: acceleration in g in
// the board's axes goes in (M5.Imu.getAccel), decisions come out.
class Tilt {
  public:
    static constexpr float kDeadZoneDeg = 4.0f; // smaller tilts do nothing
    static constexpr float kFullDeg = 30.0f;    // full turn or climb from here on
    static constexpr float kMoveDeg = 8.0f;     // what counts as moving the stick
    static constexpr float kSmoothing = 0.3f;   // share of a new reading in the filter

    // The current pose becomes neutral.
    void recenter(float ax, float ay, float az);

    // Takes a reading and updates turn() and climb(). The first reading
    // ever becomes neutral, as if recenter() had been called.
    void update(float ax, float ay, float az);

    float turn() const { return turn_; }   // -1..1, above 0 to the right
    float climb() const { return climb_; } // -1..1, above 0 up

    // Whether the stick has turned by more than kMoveDeg since the last time
    // this said yes. For the display timeout: a stick lying still, at any
    // angle, is no activity.
    bool moved(float ax, float ay, float az);

  private:
    struct Vec {
        float x, y, z;
    };

    static bool normalize(Vec &v);
    static float rollDeg(const Vec &g);
    static float pitchDeg(const Vec &g);
    static float shape(float deg);

    Vec filtered_{0, 0, 1};
    float neutralRoll_ = 0;
    float neutralPitch_ = 0;
    bool centered_ = false;

    Vec anchor_{0, 0, 1};
    bool anchored_ = false;

    float turn_ = 0;
    float climb_ = 0;
};
