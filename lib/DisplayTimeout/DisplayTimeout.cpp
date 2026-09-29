#include "DisplayTimeout.h"

DisplayTimeout::DisplayTimeout(uint32_t idleMs) : idleMs_(idleMs) {}

void DisplayTimeout::begin(uint32_t nowMs) { lastActivityMs_ = nowMs; }

bool DisplayTimeout::shouldBeOn(uint32_t nowMs, bool activity) {
    if (activity) {
        lastActivityMs_ = nowMs;
    }
    // Unsigned subtraction handles the millis() rollover correctly.
    return (nowMs - lastActivityMs_) < idleMs_;
}
