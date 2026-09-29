#include "Flight.h"

#include <math.h>

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kSpeeds[Flight::kSpeedLevels] = {12.0f, 24.0f, 48.0f}; // cells per second

// The autopilot's wandering: two slow sines that never fall in step.
constexpr float kWanderA = 0.28f; // radians per second
constexpr float kWanderB = 0.18f;

constexpr float kTurnRate = 1.3f; // radians per second at full turn input

// How fast each quantity catches up with its target, per second.
constexpr float kTurnEase = 4.0f;
constexpr float kRiseEase = 2.5f; // climbing over a ridge cannot wait
constexpr float kSinkEase = 0.8f; // coming down into a valley can
constexpr float kAttitudeEase = 3.0f;

// Seen from the cockpit: level, with the horizon across the middle of the
// screen as in the original; the nose rises in a climb, the view banks into
// turns.
constexpr float kBasePitch = 0.0f;
constexpr float kPitchPerClimb = 0.12f;
constexpr float kPitchPerRise = 0.004f; // per world unit per second of vertical speed
constexpr float kMaxPitch = 0.35f;
constexpr float kBankPerTurn = 0.2f; // per radian per second
constexpr float kMaxBank = 0.3f;

// Where the ground ahead is looked at, in seconds of flight, on top of a
// few cells that are always checked.
constexpr float kLookNear = 4.0f;
constexpr float kLookAhead[] = {0.0f, 0.4f, 0.8f, 1.4f, 2.2f};

float clampTo(float v, float limit) {
    if (v > limit) {
        return limit;
    }
    if (v < -limit) {
        return -limit;
    }
    return v;
}

float ease(float dt, float rate) {
    const float share = dt * rate;
    return share < 1.0f ? share : 1.0f;
}

float wrapMap(float v) {
    v = fmodf(v, static_cast<float>(Terrain::kSize));
    return v < 0 ? v + Terrain::kSize : v;
}

float wrapAngle(float a) {
    a = fmodf(a + kPi, 2 * kPi);
    return (a < 0 ? a + 2 * kPi : a) - kPi;
}

} // namespace

Controls combine(const Controls &a, const Controls &b) {
    Controls sum;
    sum.turn = clampTo(a.turn + b.turn, 1.0f);
    sum.climb = clampTo(a.climb + b.climb, 1.0f);
    return sum;
}

float Flight::speed() const { return kSpeeds[speedLevel_]; }

void Flight::nextSpeed() { speedLevel_ = (speedLevel_ + 1) % kSpeedLevels; }

float Flight::groundAhead(const Terrain &terrain) const {
    const float fx = camera_.forwardX();
    const float fy = camera_.forwardY();
    float highest = terrain.groundAt(camera_.x, camera_.y);
    for (float seconds : kLookAhead) {
        const float d = kLookNear + seconds * speed();
        const float h = terrain.groundAt(camera_.x + fx * d, camera_.y + fy * d);
        if (h > highest) {
            highest = h;
        }
    }
    return highest;
}

void Flight::begin(uint32_t nowMs, const Terrain &terrain) {
    lastMs_ = nowMs;
    turnRate_ = 0;
    camera_.altitude = groundAhead(terrain) + kClearance;
    camera_.pitch = kBasePitch;
    camera_.bank = 0;
}

void Flight::update(uint32_t nowMs, const Controls &controls, const Terrain &terrain) {
    // Unsigned subtraction survives the millis() rollover.
    uint32_t stepMs = nowMs - lastMs_;
    lastMs_ = nowMs;
    if (stepMs > kMaxStepMs) {
        stepMs = kMaxStepMs;
    }
    if (stepMs == 0) {
        return;
    }
    const float dt = stepMs / 1000.0f;
    clock_ += dt;

    const float turn = clampTo(controls.turn, 1.0f);
    const float climb = clampTo(controls.climb, 1.0f);

    // Course.
    const float wander =
        kWanderA * sinf(clock_ * 0.23f) + kWanderB * sinf(clock_ * 0.071f + 1.3f);
    const float wantedRate = wander + turn * kTurnRate;
    turnRate_ += (wantedRate - turnRate_) * ease(dt, kTurnEase);
    camera_.heading = wrapAngle(camera_.heading - turnRate_ * dt);

    const float step = speed() * dt;
    camera_.x = wrapMap(camera_.x + camera_.forwardX() * step);
    camera_.y = wrapMap(camera_.y + camera_.forwardY() * step);

    // Height: cruise above the highest ground ahead; a dive goes down to
    // just above it, a climb goes up to kClimbRange higher.
    const float ahead = groundAhead(terrain);
    float target = ahead + kClearance + climb * kClimbRange;
    if (target < ahead + kMinClearance) {
        target = ahead + kMinClearance;
    }
    const float before = camera_.altitude;
    const float rate = target > before ? kRiseEase : kSinkEase;
    camera_.altitude += (target - before) * ease(dt, rate);
    const float floor = terrain.groundAt(camera_.x, camera_.y) + kMinClearance;
    if (camera_.altitude < floor) {
        camera_.altitude = floor;
    }
    const float rise = (camera_.altitude - before) / dt;

    // Attitude.
    const float wantedPitch =
        clampTo(kBasePitch + climb * kPitchPerClimb + rise * kPitchPerRise, kMaxPitch);
    camera_.pitch += (wantedPitch - camera_.pitch) * ease(dt, kAttitudeEase);
    const float wantedBank = clampTo(turnRate_ * kBankPerTurn, kMaxBank);
    camera_.bank += (wantedBank - camera_.bank) * ease(dt, kAttitudeEase);
}
