#pragma once

#include <stdint.h>

// The Cardputer keyboard as a grid of held keys.
//
// Keys are addressed the way they are printed: 14 across (x), 4 rows down
// (y), from "`" at the top left to Space at the bottom right; the same layout
// M5Stack's M5Cardputer library uses.
//
// The Cardputer ADV reports its keys through a TCA8418 controller wired as a
// 7x8 matrix; decodeTca8418() turns one of its key events into this grid.
// Like everything in lib/, it never touches hardware: event bytes go in,
// held keys come out.
struct KeyPos {
    uint8_t x, y;
};

namespace CardputerKey {
// The arrows are printed on ; , . / and work without Fn here.
constexpr KeyPos kUp{11, 2};    // ;
constexpr KeyPos kLeft{10, 3};  // ,
constexpr KeyPos kDown{11, 3};  // .
constexpr KeyPos kRight{12, 3}; // /
constexpr KeyPos kW{2, 1};
constexpr KeyPos kA{2, 2};
constexpr KeyPos kS{3, 2};
constexpr KeyPos kD{4, 2};
constexpr KeyPos kEnter{13, 2};
constexpr KeyPos kSpace{13, 3};
} // namespace CardputerKey

struct KeyEvent {
    bool valid;   // false for "no event" and for GPIO events
    bool pressed; // false: released
    KeyPos pos;
};

// One byte from the TCA8418's KEY_EVENT_A register: bit 7 set on a press,
// the low 7 bits the key number, 1 + row * 10 + column in its 7x8 matrix.
KeyEvent decodeTca8418(uint8_t event);

class KeyGrid {
  public:
    static constexpr int kWidth = 14;
    static constexpr int kHeight = 4;

    void apply(const KeyEvent &event);
    void clear();

    bool held(KeyPos pos) const;
    bool anyHeld() const;

  private:
    uint16_t rows_[kHeight] = {}; // bit x of row y: the key is held
};
