# Gantry-System

This project is intended to control an x-y gantry system using a microcontroller.

## Sending G-code

Send one command per line over serial at **115200 baud**, with spaces between
the command and its parameters. The serial monitor should append a newline.
The current parser does not support inline `;` comments, so send the command
alone and keep its explanation outside the command.

Examples:

```text
10 Y20 F10
M700 S1
G21
M730 S0
```

Each received G-code command produces a result line: successful commands start
with `ok:`, and invalid or unsupported commands start with `Error:`. For
example, `G0 X40` returns `ok: G0 executed`, and `M203 X1000` returns
`ok: M203 executed` while setting the X-axis maximum feedrate to 1000 steps/s.

The parser accepts the parameter letters `X`, `Y`, `Z`, `F`, `I`, `J`, `R`,
and `S`, but not every command uses every parameter. The default coordinate
unit is raw steps. `G20` selects inches, `G21` selects millimeters, and custom
command `M730 S0` selects steps (`S1` selects millimeters and `S2` inches).
Coordinates, `F`, `M201`, and `M203` use the selected unit; feedrate is per
second and acceleration per second squared. `M92` calibration is always in
steps/mm. Movement supports X and Y coordinates; Z movement is not implemented.

## Movement commands

| Command | Example             | Current behavior                                                                                                                                          |
| ------- | ------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `G0`  | `G0 X10 Y20`      | Rapid XY move, using `G0_DEFAULT_FEEDRATE_STEPS_S` when `F` is omitted. An explicit `F` overrides the rapid default and updates the modal feedrate. |
| `G1`  | `G1 X10 Y20 F10`  | XY move using the modal feedrate; `F` is in steps/s in the default mode.                                                                                |
| `G2`  | `G2 X10 Y0 I5 J0` | Clockwise XY arc using center offsets from the arc start.                                                                                                 |
| `G3`  | `G3 X10 Y0 R5`    | Counterclockwise XY arc using a radius.                                                                                                                   |

For `G2`/`G3`, provide either `I`/`J` center offsets or `R`, but not both.
`I` and `J` are offsets from the arc's starting point; omitted offsets are
treated as zero. Positive `R` selects the minor arc and negative `R` the major
arc. Full circles are supported with `I`/`J` and no endpoint; `R` arcs need
distinct start and end points. Arcs are interpolated in the XY plane with a
step-space acceleration/deceleration profile. This is not Marlin's queued
planner/look-ahead implementation.

Coordinates are tracked from an initial logical position of `(0, 0)`. `G91`
selects relative endpoints (the default), `G90` selects absolute endpoints,
and omitted X/Y coordinates leave that axis unchanged. `G92 X0 Y0` changes the
logical coordinates without moving the gantry. Since homing is not implemented,
the initial logical position must match the gantry's actual position. A
completion melody is requested after a supported `G0`–`G3` move returns.

## Mode, position, and motor commands

| Command  | Example            | Current behavior                                                                                                        |
| -------- | ------------------ | ----------------------------------------------------------------------------------------------------------------------- |
| `G90`  | `G90`            | Use absolute X/Y endpoints.                                                                                             |
| `G91`  | `G91`            | Use relative X/Y endpoints.                                                                                             |
| `G92`  | `G92 X0 Y0`      | Set the logical current position without moving the gantry.                                                             |
| `G28`  | `G28`            | Accepted by the parser; homing is not implemented.                                                                      |
| `M17`  | `M17 X Z`        | Enable the selected X/Y/Z stepper drivers; with no selectors, enable all three. Y selects both linked Y drivers.        |
| `M18`  | `M18 Y`          | Disable the selected X/Y/Z stepper drivers; with no selectors, disable all three. Y selects both linked Y drivers.      |
| `G20`  | `G20`            | Select inches, converting the tracked position and modal feedrate without changing the physical position or speed.      |
| `G21`  | `G21`            | Select millimeters, converting the tracked position and modal feedrate without changing the physical position or speed. |
| `M730` | `M730 S0`        | Select units: `S0` steps, `S1` millimeters, `S2` inches.                                                          |
| `M92`  | `M92 X80 Y80`    | Store per-axis steps/mm calibration; omitted axes keep their current calibration.                                       |
| `M201` | `M201 X4000000 Y4000000` | Set per-axis maximum acceleration in the active coordinate unit/s².                                                    |
| `M203` | `M203 X64000 Y12000`     | Set per-axis maximum feedrate in the active coordinate unit/s; moves are capped by these limits.                        |
| `M502` | `M502`           | Accepted by the parser, but reset behavior is not implemented and the command returns an error.                         |
| `M114` | `M114`           | Report the tracked logical X and Y coordinates; this is not sensor feedback.                                            |
| `M119` | `M119`           | Endstop reporting is not implemented and the command returns an error.                                                  |

XY motion enables both X/Y drivers and leaves them enabled until an `M18` command
disables them. A later XY move re-enables both. Z motor enable control is supported,
but Z movement is not implemented.

Homing (`G28`), reset (`M502`), and endstop reporting (`M119`) do not have
functional implementations yet. `M92`, `M201`, and `M203` settings are held in
RAM and reset to their defaults after reboot. The default unit mode is steps.
The stored calibration defaults to 60 steps/mm for each axis; `M92 X80 Y80`
changes the stored X/Y calibration to 80 steps/mm. Motion limits are stored
internally as steps/s² and steps/s; `M201` and `M203` values are interpreted
in the active coordinate unit. The X/Y boot defaults are set to about 80% of
the experimentally tested no-load maxima: 4,000,000 steps/s² acceleration,
9,600 steps/s feedrate for X, and 12,000 steps/s for Y. Z limits remain at
100 steps/s² and 5 steps/s; Z calibration and limits are stored but do not
affect motion because Z movement is not implemented.

The experimentally observed no-load maximum feedrates are approximately
12,000 steps/s on X at the current acceleration setting and 15,000 steps/s on
Y. Maximum acceleration tested was 5,000,000 steps/s². These are empirical
estimates rather than guaranteed hardware limits, and may need fine-tuning
under load.

The observed maximum travel distances are approximately 10,500 steps on X and
55,000 steps on Y. These are empirical hardware measurements.

## Sound commands

The passive buzzer is connected to GPIO 4. Sound playback is scheduled by
`SoundManager::update()` without delays, and the main loop and motion stepping
service that scheduler. Buzzer output uses ESP32 LEDC PWM; the default volume
is 30% of the 50%-duty maximum. Volume can be set with `M712` or in application
code with `processor->setSoundVolume(0-100)`.

Send these commands over serial. `M700 S[int]` selects a melody or tone:

| `S value` | `M700 S[int]` action |
| ----------- | ---------------------- |
| `S1`      | Play startup melody    |
| `S2`      | Play completion melody |
| `S3`      | Play a single tone     |
| `S4`      | Play a random tone     |
| `S5`      | Play crash melody      |
| `S6`      | Play custom theme      |
| `S7`      | Play death melody      |

Other sound settings use their own `S` parameters:

| Command                    | Action                       |
| -------------------------- | ---------------------------- |
| `M710 S0` / `M710 S1`  | Disable / enable all sound   |
| `M711 S0` / `M711 S1`  | Disable / enable step sounds |
| `M712 S0`–`M712 S100` | Set sound volume (percent)   |

The startup melody plays once during boot: the six-note 1-Up fanfare
(`E6 G6 E7 C7 D7 G7`) followed by three repetitions of the coin sound
(`B5 E6`). It is scheduled non-blockingly. The completion melody plays
after a supported movement command finishes. Step sounds are off by default
and are suppressed while a melody or other tone is playing. When enabled,
their pitch follows the movement speed, rising during acceleration and
falling during deceleration and after the move. `M700 S5` requests
the crash melody manually; automatic crash playback requires crash detection
hardware and logic, which are not currently implemented.

## RGB status LED

The addressable RGB LED is connected to GPIO 48. Its patterns are timer-driven:
it flashes deep blue during boot, flashes green once after the startup melody
finishes, then settles to steady emerald green. During a movement command it stays yellow
and returns to steady green as soon as the move completes. After 10 seconds
without serial input, standby begins: the SoundManager repeats the custom theme
(`M700 S6`) non-blockingly only after sound is enabled with `M710 S1`; standby
sound is off by default. The LED continues its deep-purple 2-second, pink
2-second flash cycle regardless of the sound setting. Any incoming serial data
stops the standby melody immediately and exits standby. The crash pattern (red
flash) is available in `RgbStatus`, but automatic crash detection is not
implemented.

| Command                    | Action                                                                  |
| -------------------------- | ----------------------------------------------------------------------- |
| `M720 S0`                | Turn the RGB LED off.                                                   |
| `M720 S1`                | Turn the RGB LED on and resume its current status pattern.              |
| `M721 S0`–`M721 S255` | Set RGB brightness from off to maximum. The default after reset is 128. |

Like the sound and movement schedulers, RGB patterns are advanced by periodic
`update()` calls rather than delays. LED frame transmission itself takes a
brief synchronous transfer to the addressable LED. Brightness changes apply
immediately and are not saved across resets.
