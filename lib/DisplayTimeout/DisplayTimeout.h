#pragma once

#include <stdint.h>

// Decides when to turn the display off after a period of inactivity.
//
// It does not deal with powering off: the StickS3 side button turns the
// board off on a double press by itself, in the PMIC, without the firmware.
//
// Like everything in lib/, it does not touch the hardware: the time and
// whether a button was pressed go in, a decision comes out.
class DisplayTimeout {
  public:
    static constexpr uint32_t kIdleMs = 180000; // 3 minutes

    explicit DisplayTimeout(uint32_t idleMs = kIdleMs);

    // Sets the point the idle time is counted from. Call once at startup.
    void begin(uint32_t nowMs);

    // activity: whether any button is pressed in this tick.
    bool shouldBeOn(uint32_t nowMs, bool activity);

  private:
    uint32_t idleMs_;
    uint32_t lastActivityMs_ = 0;
};
