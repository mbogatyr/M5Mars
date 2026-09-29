#pragma once

#include <M5Unified.h>

#include "CardputerKeys.h"
#include "Flight.h"

// The keys of whichever board MARS runs on.
//
// StickS3: KEY1 makes a new planet, KEY2 changes the speed.
// Cardputer ADV: the G0 button or Enter make a new planet, Space changes the
// speed, and the arrow keys (; , . /) or W A S D steer on top of the tilt.
// Its keyboard is a TCA8418 on the internal I2C bus, polled every tick; on
// other boards it is never touched.
class Input {
  public:
    // Call after M5.begin().
    void begin();

    // Call once per loop, after M5.update().
    void update();

    bool newPlanet() const { return newPlanet_; } // pressed in this tick
    bool nextSpeed() const { return nextSpeed_; } // pressed in this tick
    bool held() const { return held_; }           // any key down: activity
    Controls keys() const { return keys_; }       // the arrow keys
    bool hasKeyboard() const { return keyboard_; }

    // Presses from serial commands, for testing; they last one tick.
    void pressNewPlanet() { newPlanet_ = true; }
    void pressNextSpeed() { nextSpeed_ = true; }

  private:
    void pollKeyboard();

    bool keyboard_ = false;
    KeyGrid grid_;
    bool enterWas_ = false;
    bool spaceWas_ = false;

    bool newPlanet_ = false;
    bool nextSpeed_ = false;
    bool held_ = false;
    Controls keys_;
};
