# M5Burner listings

MARS is published on M5Burner as two listings, because a listing there has a
single device and is identified by its name (see below). Both use the same
image: it recognises the board at start-up.

| | StickS3 | Cardputer ADV |
|---|---|---|
| Name | MARS | MARS for Cardputer ADV |
| Category | Games | Games |
| Device | StickS3 | Cardputer ADV |
| Version | v1.0.0 | v1.1.0 |
| File | `dist/MARS-v1.0.0.bin` | `dist/MARS-v1.1.0.bin` |
| Cover | a StickS3 screenshot (cover 5 of 5) | `dist/MARS-cover.png`, a Cardputer ADV screenshot |
| Visibility | Public | Public |
| Uploaded | 2026-09-29, "Pending" review | 2026-09-29, "Pending" review |

Project link for both: https://github.com/mbogatyr/M5Mars. Files are full
images flashed at 0x0. `dist/` is not in git; rebuild it as described in
CLAUDE.md, "Publishing to M5Burner". `docs/screenshot.png` is the Cardputer
ADV cover.

How M5Burner behaves, found out while uploading:
- "Supported devices" takes one device; choosing a second replaces the first.
- A listing is found by its name. Uploading with a name that already exists
  adds a version to that listing, and the server refuses it if the public
  data differ ("新增版本不能修改固件说明，请使用固件编辑接口": a new version
  cannot change the firmware description, use the firmware edit page).
- While a listing waits for review, it offers "Edit firmware public data",
  "Edit version" and "Withdraw", but no "New version".

The StickS3 listing still carries v1.0.0. v1.1.0 runs the same on the
StickS3 (checked); it can be added there as a new version once the listing
is reviewed, with the same public data.

## StickS3: description (as uploaded)

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

Version description (v1.0.0):

```markdown
First release.

- Diamond-Square planet and voxel renderer; the sky and colours are matched to a capture of the original MARS.EXE.
- Tilt steering with the built-in accelerometer; an autopilot with terrain following.
- KEY1: new planet. KEY2: three speeds.
- About 35 fps on the StickS3.
```

## Cardputer ADV: description (as uploaded)

```markdown
**MARS** is a port of the legendary demo by **Tim Clarke** (1993) to the Cardputer ADV.

Preserved in the Hornet demoscene archive, the original MARS.EXE was a tiny DOS
program of just 5.6 KB. It generated a fractal landscape with the Diamond-Square
algorithm and flew you over it in real time with voxel rendering: a
three-dimensional "red planet" on an IBM PC 386 with VGA (320×200, 256 colours).
Every run made a new planet.

This port does the same on the Cardputer's 240×135 screen at about 33 frames per second:

- a new 256×256 Diamond-Square planet at every start, wrapping seamlessly, so the flight never ends;
- voxel rendering front to back, column by column, as in the original, with its palette,
  its salmon-red cloudy sky, the glowing horizon and far mountains sinking into dark maroon;
- an autopilot that follows the terrain; steer it by tilting the Cardputer or with the arrow keys.

### Controls

Hold the Cardputer in front of you, screen towards you. Lower the left or right end
to turn; tip the top edge away or towards you to dive or climb.

| Input | Action |
|---|---|
| Arrow keys `,` `/` or `A` `D` | Turn left / right |
| Arrow keys `;` `.` or `W` `S` | Climb / dive |
| Enter or the G0 button | New planet; the current grip becomes neutral |
| Space | Speed: slow, cruise, fast |

The keys and the tilt work together. Let go, and the autopilot flies on by itself.
The screen goes dark after 3 minutes without a key press or a tilt; tilt the
Cardputer or press a key to wake it.

The same firmware also runs on the M5StickS3 (a separate MARS listing).

Source code: https://github.com/mbogatyr/M5Mars
```

Version description (v1.1.0):

```markdown
First release for the Cardputer ADV.

- Diamond-Square planet and voxel renderer; the sky and colours are matched to a capture of the original MARS.EXE.
- Tilt steering with the built-in accelerometer, plus the keyboard: arrow keys (, / ; .) or W A S D.
- Enter or G0: new planet. Space: three speeds.
- About 33 fps on the Cardputer ADV.
```
