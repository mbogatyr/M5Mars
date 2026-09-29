#include "CardputerKeys.h"

namespace {

// The TCA8418 matrix: 7 rows by 8 columns in use, numbered 10 to a row.
constexpr int kMatrixRows = 7;
constexpr int kMatrixColumns = 8;
constexpr int kNumbersPerRow = 10;

} // namespace

KeyEvent decodeTca8418(uint8_t event) {
    const int number = (event & 0x7F) - 1;
    const int row = number / kNumbersPerRow;
    const int column = number % kNumbersPerRow;
    if (number < 0 || row >= kMatrixRows || column >= kMatrixColumns) {
        return {false, false, {0, 0}};
    }
    // Each matrix row is two printed columns: its first four lines are the
    // left one, from the top row of keys down, the next four the right one.
    const KeyPos pos{static_cast<uint8_t>(row * 2 + (column > 3 ? 1 : 0)),
                     static_cast<uint8_t>(column % 4)};
    return {true, (event & 0x80) != 0, pos};
}

void KeyGrid::apply(const KeyEvent &event) {
    if (!event.valid || event.pos.x >= kWidth || event.pos.y >= kHeight) {
        return;
    }
    const uint16_t bit = static_cast<uint16_t>(1u << event.pos.x);
    if (event.pressed) {
        rows_[event.pos.y] |= bit;
    } else {
        rows_[event.pos.y] &= static_cast<uint16_t>(~bit);
    }
}

void KeyGrid::clear() {
    for (auto &row : rows_) {
        row = 0;
    }
}

bool KeyGrid::held(KeyPos pos) const {
    if (pos.x >= kWidth || pos.y >= kHeight) {
        return false;
    }
    return (rows_[pos.y] >> pos.x) & 1;
}

bool KeyGrid::anyHeld() const {
    for (auto row : rows_) {
        if (row != 0) {
            return true;
        }
    }
    return false;
}
