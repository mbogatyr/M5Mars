# M5Burner listing

What goes into the upload form at
burner.m5stack.com/developer/firmware/upload.

| Field | Value |
|---|---|
| Name | MARS |
| Category | Games |
| Devices | StickS3, Cardputer ADV (from v1.1.0) |
| Version | v1.1.0 |
| Project link | https://github.com/mbogatyr/M5Mars |
| File | `dist/MARS-v1.1.0.bin` (a full image, flashed at 0x0; the same file for both devices) |
| Cover | `dist/MARS-cover.png` (a screenshot from the Cardputer ADV, picked by the user) |
| Visibility | Public (needs moderation) |

`dist/` is not in git; rebuild it as described in CLAUDE.md, "Publishing to
M5Burner". The cover is also kept as `docs/screenshot.png`.

History:
- v1.0.0, uploaded on 2026-09-29: StickS3 only, with the description below
  minus the Cardputer ADV parts and another cover (a StickS3 screenshot).
  Visibility Public, status "Pending" right after the upload.
- v1.1.0, 2026-09-29: adds the Cardputer ADV.

## Description

```markdown
**MARS** is a port of the legendary demo by **Tim Clarke** (1993) to the M5StickS3 and the Cardputer ADV.

Preserved in the Hornet demoscene archive, the original MARS.EXE was a tiny DOS
program of just 5.6 KB. It generated a fractal landscape with the Diamond-Square
algorithm and flew you over it in real time with voxel rendering: a
three-dimensional "red planet" on an IBM PC 386 with VGA (320×200, 256 colours).
Every run made a new planet.

This port does the same on the 240×135 screen at 33–35 frames per second:

- a new 256×256 Diamond-Square planet at every start, wrapping seamlessly, so the flight never ends;
- voxel rendering front to back, column by column, as in the original, with its palette,
  its salmon-red cloudy sky, the glowing horizon and far mountains sinking into dark maroon;
- an autopilot that follows the terrain, and steering by tilting the device.

One image runs on both devices: it recognises the board at start-up.

### Controls

Hold the device in landscape, screen towards you. Lower the left or right end
to turn; tip the top edge away or towards you to dive or climb.

**StickS3**

| Input | Action |
|---|---|
| KEY1 (the blue button on the front) | New planet; the current grip becomes neutral |
| KEY2 (the button on the edge) | Speed: slow, cruise, fast |

**Cardputer ADV**

| Input | Action |
|---|---|
| Arrow keys `,` `/` or `A` `D` | Turn left / right |
| Arrow keys `;` `.` or `W` `S` | Climb / dive |
| Enter or the G0 button | New planet; the current grip becomes neutral |
| Space | Speed: slow, cruise, fast |

Let go, and the autopilot flies on by itself. The screen goes dark after 3 minutes
without a key press or a tilt; tilt the device or press a key to wake it.

Source code: https://github.com/mbogatyr/M5Mars
```

## Version description (v1.1.0)

```markdown
Cardputer ADV support.

- The same image now runs on the Cardputer ADV: tilt steering with its accelerometer, plus the keyboard.
- Arrow keys (, / ; .) or W A S D steer, Enter or G0 makes a new planet, Space changes the speed.
- About 33 fps on the Cardputer ADV, 35 on the StickS3.
```

## Version description (v1.0.0)

```markdown
First release.

- Diamond-Square planet and voxel renderer; the sky and colours are matched to a capture of the original MARS.EXE.
- Tilt steering with the built-in accelerometer; an autopilot with terrain following.
- KEY1: new planet. KEY2: three speeds.
- About 35 fps on the StickS3.
```
