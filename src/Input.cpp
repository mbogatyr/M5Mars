#include "Input.h"

namespace {

// The Cardputer ADV keyboard controller, set up as M5Stack's M5Cardputer
// library does it, but polled instead of interrupt-driven.
constexpr uint8_t kTcaAddress = 0x34;
constexpr uint32_t kTcaFreq = 400000;
constexpr uint8_t kRegCfg = 0x01;
constexpr uint8_t kRegIntStat = 0x02;
constexpr uint8_t kRegKeyLckEc = 0x03; // low 4 bits: events queued
constexpr uint8_t kRegKeyEventA = 0x04;
constexpr uint8_t kRegKpGpio1 = 0x1D; // rows in the keypad matrix
constexpr uint8_t kRegKpGpio2 = 0x1E; // columns 0-7
constexpr uint8_t kRegKpGpio3 = 0x1F; // columns 8-9
constexpr uint8_t kCfgKeyEvents = 0x01;
constexpr int kQueueDepth = 10;

m5::I2C_Device tca(kTcaAddress, kTcaFreq, &m5::In_I2C);

float axis(bool plus, bool minus) { return (plus ? 1.0f : 0.0f) - (minus ? 1.0f : 0.0f); }

} // namespace

void Input::begin() {
    keyboard_ = M5.getBoard() == m5::board_t::board_M5CardputerADV;
    if (!keyboard_) {
        return;
    }
    tca.writeRegister8(kRegKpGpio1, 0x7F); // 7 rows
    tca.writeRegister8(kRegKpGpio2, 0xFF); // 8 columns
    tca.writeRegister8(kRegKpGpio3, 0x00);
    tca.bitOn(kRegCfg, kCfgKeyEvents);
    // Drop whatever was queued before.
    for (int i = 0; i < kQueueDepth && tca.readRegister8(kRegKeyEventA) != 0; ++i) {
    }
    tca.writeRegister8(kRegIntStat, 0x03);
}

void Input::pollKeyboard() {
    for (int i = 0; i < kQueueDepth; ++i) {
        if ((tca.readRegister8(kRegKeyLckEc) & 0x0F) == 0) {
            break;
        }
        grid_.apply(decodeTca8418(tca.readRegister8(kRegKeyEventA)));
    }
    tca.writeRegister8(kRegIntStat, 0x01);
}

void Input::update() {
    newPlanet_ = M5.BtnA.wasPressed();
    nextSpeed_ = M5.BtnB.wasPressed();
    held_ = M5.BtnA.isPressed() || M5.BtnB.isPressed();
    keys_ = Controls{};
    if (!keyboard_) {
        return;
    }

    pollKeyboard();
    using namespace CardputerKey;
    const bool enter = grid_.held(kEnter);
    const bool space = grid_.held(kSpace);
    newPlanet_ = newPlanet_ || (enter && !enterWas_);
    nextSpeed_ = nextSpeed_ || (space && !spaceWas_);
    enterWas_ = enter;
    spaceWas_ = space;

    keys_.turn = axis(grid_.held(kRight) || grid_.held(kD), grid_.held(kLeft) || grid_.held(kA));
    keys_.climb = axis(grid_.held(kUp) || grid_.held(kW), grid_.held(kDown) || grid_.held(kS));
    held_ = held_ || grid_.anyHeld();
}
