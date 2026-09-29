# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

MARS for the M5StickS3: a real-time voxel flight over a fractal red planet,
after Tim Clarke's MARS.EXE (1993, 5.6 KB, VGA 320x200x256). The planet is a
256x256 Diamond-Square height map on a torus, shaded by slope, coloured with
the palette of the original; the view is drawn front to back column by column
with a y-buffer (the Comanche technique), sinks into dark maroon with
distance and has a ceiling of fractal clouds down to a glowing horizon.
`mars.c` in the root is the reference sketch the user supplied (DOS, Turbo
C); it is not built and stays as it is.

The look of the original is taken from a 320x200 capture of MARS.EXE:
https://www.youtube.com/watch?v=ZCUpqprm3g4 (70 s). Measured there on
2026-09-29, and what the code now follows: the camera flies level with the
horizon across the middle of the screen; the clear sky is a saturated
salmon-red (~216, 66, 50) with soft pink clouds (~225, 155, 145) that thin
into streaks and run right down to the horizon, the sky growing lighter
towards it; the horizon is a bright line (~240, 130, 104); the far ground is
dark maroon (~50, 4, 0), not a light haze; near ground is orange-brown with
lit slopes (~125, 18, 0 up to ~185, 70, 35). An earlier version with a dark
brown sky and a light salmon haze band was rejected by the user as unlike
the original.

The autopilot flies by itself; tilting the stick steers (see Controls).

## Language

All documentation and the project itself are kept in English: this file, code
comments, identifiers, strings shown on the display, commit messages and any
other text files. New text is written in English too, even when the
conversation with the user happens in another language.

## Commands

PlatformIO is not installed globally but by the official installer into a
venv. The binary lives at `~/.platformio/penv/bin/pio`; it is not on `PATH`,
so it has to be called by its full path.

```bash
~/.platformio/penv/bin/pio test -e native                         # host unit tests for the logic
~/.platformio/penv/bin/pio test -e native -f test_voxel_renderer  # a single test suite
~/.platformio/penv/bin/pio run -e sticks3                         # build the firmware
~/.platformio/penv/bin/pio run -e sticks3 -t upload               # flash the board
~/.platformio/penv/bin/pio run -e sticks3 -t merged               # single image for M5Burner
~/.platformio/penv/bin/pio device monitor -e sticks3              # serial monitor, 115200
```

`upload_port` and `monitor_port` are pinned to `/dev/cu.usbmodem*`: when the
board is not on USB, PlatformIO otherwise picks
`/dev/cu.Bluetooth-Incoming-Port` and fails with "No serial data received".

The xtensa-esp32s3 toolchain is installed into `~/.platformio/packages` once
per machine (about eight minutes) and is shared by all projects. The first
build of a new project downloads the latest M5Unified into `.pio/` and takes
about 20 seconds.

### Preview on the Mac

`tools/preview/` flies over a planet with the real `lib/` code and saves a
contact sheet of six frames (cruise, right, left, climb, dive, fast), to check
the look without flashing:

```bash
c++ -std=gnu++17 -O2 -Ilib/Camera -Ilib/Terrain -Ilib/MarsPalette \
    -Ilib/VoxelRenderer -Ilib/Flight tools/preview/main.cpp \
    lib/Terrain/Terrain.cpp lib/MarsPalette/MarsPalette.cpp \
    lib/VoxelRenderer/VoxelRenderer.cpp lib/Flight/Flight.cpp -lz -o /tmp/mars_preview
/tmp/mars_preview /tmp/sheet.png [seed]
```

## Architecture

The split into `lib/` and `src/` is not cosmetic here, it is load-bearing:

- `lib/` is the logic: plain C++ with no Arduino, no M5Unified and no hardware
  access of any kind.
  - `Terrain`: `diamondSquare()` and `smoothTorus()` on a wrapping grid; the
    planet (heights, a colour index per cell with slope shading and grain,
    packed as `color << 8 | height`) and a 128x128 cloud texture.
  - `MarsPalette`: the mars.c ramp (`r=i/4+16, g=i/8, b=i/16` at 6 bits),
    the far-ground, sky, cloud and horizon colours; lookup tables for 16
    distance levels and the sky.
  - `Camera`: position, heading, altitude, pitch, bank. Forward is
    `(cos, sin)`, right is `(sin, -cos)`: turning right makes the heading
    smaller.
  - `VoxelRenderer`: draws a frame into any RGB565 buffer.
  - `Flight`: autopilot, terrain following, speed levels, controls.
  - `Tilt`: accelerometer readings to turn/climb, and "the stick moved".
  - `DisplayTimeout`: turns the display off after 3 minutes without activity.
- `src/` is everything that knows about the board: `Renderer` owns the sprite
  and pushes it, `main.cpp` wires the logic to the hardware and takes serial
  commands.

The `native` environment builds only `lib/` (PlatformIO's `test_build_src`
defaults to `no`), so the logic is tested on the Mac without the board.
**Do not pull hardware dependencies into `lib/`: that breaks the tests.**

### Time is passed in as a parameter

The logic in `lib/` does not call `millis()` itself; it receives the current
time as an argument. That way tests can substitute any moment without waiting.
`loop()` has no `delay()` while the display is on: drawing a frame paces it.
With the display off it runs at 50 Hz.

`millis()` overflows after roughly 49 days. Compute intervals with unsigned
subtraction `now - since`, so the overflow goes unnoticed. `Flight` flies a
gap longer than 100 ms as 100 ms, so the camera does not jump after the
display sleeps.

### Rendering and memory

`VoxelRenderer` writes straight into the buffer of a 16-bit `M5Canvas`
(240x135, landscape, `setRotation(1)`), then `Renderer` pushes it with a
single `pushSprite`. The sprite keeps RGB565 with its bytes swapped, so the
board's renderer is built with `VoxelRenderer(true)`.

Everything the renderer reads at random must be in internal RAM: the terrain
(144 KB) is a static object, the sprite (65 KB) is created with
`setPsram(false)`. Through the PSRAM cache the same work is much slower
(M5TalkingTom measured 26 ms instead of a few for one frame). Static RAM is at
56 % of 320 KB; watch it when adding tables.

GCC on the board rejects brace-initialising a struct that has default member
values (`Camera camera_{128, 128}`), although clang on the Mac accepts it.

## Controls

The stick is held in landscape, screen towards the pilot.

| Input | Action |
|---|---|
| Tilt left/right (lift one end) | turn; the view banks into the turn |
| Tip the top edge | climb or dive (dive goes down to just above the ground) |
| KEY1 | new planet, and the current grip becomes neutral |
| KEY2 | speed: slow, cruise, fast |
| Side button | the PMIC's own: power on, double press off (below) |

Letting go of the tilt hands the flight back to the autopilot. The neutral
grip is taken 0.7 s after start-up and after the display wakes. A tilt of
more than 8° counts as activity for the display timeout, so a stick lying
still at any angle does not keep the screen on; a press or a tilt that wakes
the display does nothing else.

## Self-testing on the board

`tools/serial_cmd.py` talks to the firmware over USB Serial without resetting
it (see "If the board is stuck in the bootloader" below):

```bash
PY=~/.platformio/penv/bin/python
$PY tools/serial_cmd.py snap screen.png        # screenshot, 3x
$PY tools/serial_cmd.py send perf --wait 5     # fps, paint and push time, free RAM
$PY tools/serial_cmd.py send acc --wait 5      # accelerometer and tilt at 10 Hz
$PY tools/serial_cmd.py send st                # seed, speed, camera, free RAM
$PY tools/serial_cmd.py send k1                # press KEY1 (k2: KEY2)
```

`perf` and `acc` are toggles: sending them again turns them off. When nobody
reads the port those lines are skipped, so they cannot stall the loop.

`Serial.begin(115200)` must be called in `setup()`: M5Unified's
`serial_baudrate` defaults to 0, and without it the port stays silent.

Measured on 2026-09-29 with a view distance of 480 cells: 34–36 fps;
drawing a frame takes 14 ms, pushing it over SPI another 13 ms; 130 KB of
internal RAM stay free.

## Board specifics

M5StickS3 is an ESP32-S3-PICO-1-N8R8 with 8 MB of flash, 8 MB of octal PSRAM
and an ST7789P3 135x240 display.

- PlatformIO has **no** `m5stack-sticks3` board id. The project uses
  `esp32-s3-devkitc-1` plus `board_build.arduino.memory_type = qio_opi` and
  the `default_8MB.csv` partitions. Do not "fix" this to a non-existent id.
- USB is native, with no CH9102 bridge, so on macOS the port is called
  `/dev/cu.usbmodem*`, not `/dev/cu.usbserial*`. Serial output needs the
  `-DARDUINO_USB_CDC_ON_BOOT=1` flag, which is already set.
- Buttons: KEY1 on G11 (`M5.BtnA`), KEY2 on G12 (`M5.BtnB`). Grove (G9/G10)
  and HAT2 (G1–G8, G43, G44) are free.

### Publishing to M5Burner

M5Burner writes the uploaded file starting at address 0x0, so it needs a full
image. A bare `firmware.bin` is meant for address 0x10000: written at 0x0, it
overwrites the bootloader. `pio run -e sticks3 -t merged` (the extra script
`tools/merged_image.py`) merges the bootloader, the partition table,
`boot_app0` and the application into `.pio/build/sticks3/firmware-merged.bin`
with esptool `merge_bin`. The script takes the addresses and flash parameters
(dio, 80m, 8MB) from PlatformIO's regular upload settings, so the image matches
what `upload` writes.

How this is known. Checked in M5SpectrumAnalyzer on 2026-09-27: of the six
StickS3 firmwares on burner.m5stack.com, five, including the official
UIFlow2.0, are full images. Each has the bootloader at 0x0 (header
`e9 03 02 3f`), the partition table at 0x8000 (`aa 50`) and the application at
0x10000. One firmware was uploaded as a bare application. The merged image was
tested on the board: flashed on its own, at address 0x0, with esptool.

The upload form is at burner.m5stack.com/developer/firmware/upload. It asks for:
- a name, a category and the supported devices (StickS3);
- a firmware description and a version description, both in Markdown;
- the version number and a link to the project;
- the `.bin` file;
- visibility: Public requires moderation;
- a cover image: a screenshot of the screen works.

For MARS the filled-in form (name, description and version description in
Markdown) is `docs/m5burner.md`. The files go into `dist/` (ignored by git,
as in the sibling projects; a file picker can't easily reach the hidden
`.pio/`):

```bash
~/.platformio/penv/bin/pio run -e sticks3 -t merged
cp .pio/build/sticks3/firmware-merged.bin dist/MARS-v<version>.bin
~/.platformio/penv/bin/python tools/serial_cmd.py snap dist/MARS-cover.png  # 3x screenshot
```

v1.0.0 (2026-09-29): the image was flashed with `esptool.py write_flash 0x0`,
the way M5Burner does it, and the board booted and flew at 35 fps. The cover
(the user picked it out of five board screenshots) is also kept as
`docs/screenshot.png`. Uploaded the same day through the user's Chrome,
where they are logged in (Claude in Chrome can attach files; the built-in
browser pane cannot): category Games, StickS3, Public, "Pending" review.
The form shows a cover editor (crop, offset, optional caption); the cover
went in as it is, with no caption.

Only the StickS3 is supported for now. The same ESP32-S3 image could also
run on the Cardputer and Cardputer ADV, since M5GFX detects them and drives
their 240x135 ST7789 on the same 40 MHz bus, and the framework boots without
PSRAM (`CONFIG_SPIRAM_IGNORE_NOTFOUND`). What they would need: keyboard
steering (M5Unified reads only their G0 button; the original Cardputer has no
IMU, the ADV has one on I2C G8/G9), and not touching G11, which is KEY1 on
the StickS3 but a keyboard line on the Cardputer. The StickC Plus / Plus2 are
classic ESP32 and need a separate build; the 144 KB static terrain probably
does not fit their static DRAM as it is. Not started: there was no device to
test on.

### The side button is handled by the PMIC, not the firmware

| Action | Result |
|---|---|
| Single press | Power on / reset |
| Double press | Power off |
| Long hold | Download mode (the internal green LED blinks) |

So the firmware does not need its own power-off button. If powering off from
software is ever needed (for example on idle), `M5.Power.powerOff()` used to
wake the StickS3 right away by timer —
[M5Unified#235](https://github.com/m5stack/M5Unified/issues/235), fixed in
0.2.23. This has not been checked on a board with the fixed version.

`M5.Power` does not set `_wakeupPin` for the StickS3, so there is no
ready-made wake-up from deep sleep by button; it would have to be configured
manually with `esp_sleep_enable_ext0_wakeup`.

### If flashing fails

`A fatal error occurred: Failed to connect to ESP32-S3: No serial data received.`

The board shows up as `USB JTAG_serial debug unit` (VID 0x303A, PID 0x1001):
that is the built-in USB-Serial-JTAG, not a CDC port (a consequence of
`ARDUINO_USB_MODE=1`). Auto-reset into download mode through it does not
always work, and neither `--before usb_reset` nor `--before no_reset` helps.
The only fix is manual: hold the side button until the green LED blinks.

After flashing in that mode, "Hard resetting via RTS pin" does not start the
firmware: the chip stays in the bootloader (the port answers nothing;
`esptool.py --before no_reset --after no_reset chip_id` connects and says
"Staying in bootloader"). Neither `esptool run` nor anything else from the
Mac gets it out; powering the stick off and on with the side button does
(checked 2026-09-29; a single short press should too, see below). Flashing a
board that is already running the firmware usually needs no button at all:
the auto-reset into the loader works then, and the firmware starts after it.

### If the board is stuck in the bootloader

The firmware does not start, and the port shows `boot:0x0 (DOWNLOAD(USB/UART0))`
and `waiting for download`. This happened when a pyserial script opened and
closed the port to check on it: macOS toggles DTR/RTS when doing so, and
USB-Serial-JTAG takes that as a command to enter the bootloader. The way out is
a single short press of the side button. So checking the firmware by opening
the port from a script is best avoided; `pio device monitor` has not been
checked for this effect.

## Tests

Unit tests cover the logic in `lib/`, with one directory
`test/test_<module>/test_main.cpp` per module. `VoxelRenderer` is tested on
small frames over hand-made ground (`Terrain::flatten()` and `set()`): sky and
ground either side of the horizon, occlusion, distance darkening, the
horizon glow, pitch, bank, heading. The view reaches past the 256-cell map,
so far copies of a feature can appear in a test frame: check the near,
unfogged colour when that matters.
`src/Renderer` is checked by eye on the board and with the preview: do not
try to write tests for it; that would require mocking all of LovyanGFX and
would prove nothing useful.

`Terrain` and `VoxelRenderer` are large: declare them `static` in tests, not
on the stack.

Unity's `TEST_ASSERT_GREATER_THAN` and friends compare integers: a float
argument is truncated, so `0.3f` becomes 0 and the check quietly passes or
fails for the wrong reason. Use the `_FLOAT` variants for floats.

`main` in the tests returns the number of failures from `UNITY_END()`, and
PlatformIO reports a non-zero exit code as a signal number. A line like
`Program received signal SIGALRM` with failing tests is a reporting artifact,
not a separate problem; it disappears once the tests pass.
