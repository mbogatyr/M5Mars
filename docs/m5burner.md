# M5Burner listing

What goes into the upload form at
burner.m5stack.com/developer/firmware/upload.

| Field | Value |
|---|---|
| Name | MARS |
| Category | Games |
| Devices | StickS3 |
| Version | v1.0.0 |
| Project link | https://github.com/mbogatyr/M5Mars |
| File | `dist/MARS-v1.0.0.bin` (a full image, flashed at 0x0) |
| Cover | `dist/MARS-cover.png` (a screenshot from the board) |
| Visibility | Public (needs moderation) |

`dist/` is not in git; rebuild it as described in CLAUDE.md, "Publishing to
M5Burner". The cover is also kept as `docs/screenshot.png`.

v1.0.0 was uploaded on 2026-09-29 with exactly these values (visibility
Public, so it waits for M5Stack's review; status "Pending" right after the
upload).

## Description

```markdown
**MARS** is a port of the legendary demo by **Tim Clarke** (1993) to the M5StickS3.

Preserved in the Hornet demoscene archive, the original MARS.EXE was a tiny DOS
program of just 5.6 KB. It generated a fractal landscape with the Diamond-Square
algorithm and flew you over it in real time with voxel rendering: a
three-dimensional "red planet" on an IBM PC 386 with VGA (320×200, 256 colours).
Every run made a new planet.

This port does the same on the stick's 240×135 screen at about 35 frames per second:

- a new 256×256 Diamond-Square planet at every start, wrapping seamlessly, so the flight never ends;
- voxel rendering front to back, column by column, as in the original, with its palette,
  its salmon-red cloudy sky, the glowing horizon and far mountains sinking into dark maroon;
- an autopilot that follows the terrain, and steering by tilting the stick.

### Controls

Hold the stick in landscape, screen towards you.

| Input | Action |
|---|---|
| Lower the left or right end | Turn |
| Tip the top edge away / towards you | Dive / climb |
| KEY1 (the blue button on the front) | New planet; the current grip becomes neutral |
| KEY2 (the button on the edge) | Speed: slow, cruise, fast |

Let go, and the autopilot flies on by itself. The screen goes dark after 3 minutes
without a key press or a tilt; tilt the stick or press a key to wake it.
Double-press the power button to switch the stick off.

Source code: https://github.com/mbogatyr/M5Mars
```

## Version description (v1.0.0)

```markdown
First release.

- Diamond-Square planet and voxel renderer; the sky and colours are matched to a capture of the original MARS.EXE.
- Tilt steering with the built-in accelerometer; an autopilot with terrain following.
- KEY1: new planet. KEY2: three speeds.
- About 35 fps on the StickS3.
```
