#include "Tilt.h"

#include <math.h>

namespace {

constexpr float kDegPerRad = 57.2957795f;

// Which way round the board axes map onto turning and climbing in the
// landscape grip (setRotation(1)). Checked on the board on 2026-09-29:
// lowering the left end turns left, tipping the top edge away dives.
constexpr float kTurnSign = 1.0f;
constexpr float kClimbSign = 1.0f;

float wrap180(float deg) {
    while (deg > 180.0f) {
        deg -= 360.0f;
    }
    while (deg < -180.0f) {
        deg += 360.0f;
    }
    return deg;
}

} // namespace

bool Tilt::normalize(Vec &v) {
    const float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    // A reading far from 1 g is a throw or a sensor glitch: better ignored.
    if (length < 0.3f) {
        return false;
    }
    v = {v.x / length, v.y / length, v.z / length};
    return true;
}

// Lifting one end of the stick moves gravity along its long axis.
float Tilt::rollDeg(const Vec &g) {
    return atan2f(g.y, sqrtf(g.x * g.x + g.z * g.z)) * kDegPerRad;
}

// Tipping the top edge turns gravity around the long axis, between X and Z.
float Tilt::pitchDeg(const Vec &g) { return atan2f(g.x, g.z) * kDegPerRad; }

// Dead zone, then linear up to full deflection.
float Tilt::shape(float deg) {
    const float magnitude = fabsf(deg);
    if (magnitude <= kDeadZoneDeg) {
        return 0;
    }
    float share = (magnitude - kDeadZoneDeg) / (kFullDeg - kDeadZoneDeg);
    if (share > 1.0f) {
        share = 1.0f;
    }
    return deg > 0 ? share : -share;
}

void Tilt::recenter(float ax, float ay, float az) {
    Vec g{ax, ay, az};
    if (!normalize(g)) {
        return;
    }
    filtered_ = g;
    neutralRoll_ = rollDeg(g);
    neutralPitch_ = pitchDeg(g);
    centered_ = true;
    turn_ = 0;
    climb_ = 0;
}

void Tilt::update(float ax, float ay, float az) {
    Vec g{ax, ay, az};
    if (!normalize(g)) {
        return;
    }
    if (!centered_) {
        recenter(ax, ay, az);
        return;
    }
    filtered_.x += (g.x - filtered_.x) * kSmoothing;
    filtered_.y += (g.y - filtered_.y) * kSmoothing;
    filtered_.z += (g.z - filtered_.z) * kSmoothing;

    turn_ = kTurnSign * shape(rollDeg(filtered_) - neutralRoll_);
    climb_ = kClimbSign * shape(wrap180(pitchDeg(filtered_) - neutralPitch_));
}

bool Tilt::moved(float ax, float ay, float az) {
    Vec g{ax, ay, az};
    if (!normalize(g)) {
        return false;
    }
    if (!anchored_) {
        anchor_ = g;
        anchored_ = true;
        return false;
    }
    static const float kMoveCos = cosf(kMoveDeg / kDegPerRad);
    const float dot = g.x * anchor_.x + g.y * anchor_.y + g.z * anchor_.z;
    if (dot < kMoveCos) {
        anchor_ = g;
        return true;
    }
    return false;
}
