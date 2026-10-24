# Gantry-System
This project is intended to control an x-y gantry system using a microcontroller.

## Sending G-code

Send one command per line over serial at **115200 baud**, with spaces between
the command and its parameters. The serial monitor should append a newline.
The current parser does not support inline `;` comments, so send the command
alone and keep its explanation outside the command.

Examples:

```text
G1 X10 Y20 F600
M700 S1
```

The parser accepts the parameter letters `X`, `Y`, `Z`, `F`, `I`, `J`, `R`,
and `S`, but not every command uses every parameter. Currently, movement
supports X and Y coordinates and F feedrate; Z is not implemented.

## Movement commands

| Command | Example | Current behavior |
| --- | --- | --- |
| `G0` | `G0 X10 Y20` | Rapid XY move, using `G0_DEFAULT_FEEDRATE_MM` when `F` is omitted. An explicit `F` overrides the rapid default and updates the modal feedrate. |
| `G1` | `G1 X10 Y20 F600` | XY move using the modal feedrate; `F` sets it in units per minute. |
| `G2` | `G2 X10 Y20 I2 J0` | Accepted by the parser, but motion code reports that arcs are not implemented. |
| `G3` | `G3 X10 Y20 I2 J0` | Accepted by the parser, but motion code reports that arcs are not implemented. |

The current motion handler uses `(0, 0)` as the start of each move rather than
tracking the previous endpoint. If an axis is omitted, its destination is
treated as zero. These are current implementation limitations, not standard
G-code behavior. A completion melody is requested when a supported `G0` or
`G1` move returns.

## Mode, position, and motor commands

| Command | Example | Current behavior |
| --- | --- | --- |
| `G90` | `G90` | Accepted, but absolute positioning mode is not implemented. |
| `G91` | `G91` | Accepted, but relative positioning mode is not implemented. |
| `G92` | `G92 X0 Y0` | Accepted by the parser; setting the current position is not implemented. |
| `G28` | `G28` | Accepted by the parser; homing is not implemented. |
| `M17` | `M17` | Accepted by the parser; motor-enable command handling is not implemented. |
| `M18` | `M18` | Accepted by the parser; motor-disable command handling is not implemented. |
| `M201` | `M201 X100 Y100` | Accepted by the parser; acceleration-setting command handling is not implemented. |
| `M203` | `M203 X1000 Y1000` | Accepted by the parser; maximum-feedrate setting is not implemented. |
| `M502` | `M502` | Accepted and routed to the modifier handler, which currently only prints a message; reset behavior is not implemented. |
| `M114` | `M114` | Accepted, but position reporting is not implemented; currently only prints a handler message. |
| `M119` | `M119` | Accepted, but endstop reporting is not implemented; currently only prints a handler message. |

These commands currently pass parser validation, but are not all actionable.
Commands without a working handler may end with an “unrecognized command”
message.

## Sound commands

The passive buzzer is connected to GPIO 4. Sound playback is scheduled by
`SoundManager::update()` without delays, and the main loop and motion stepping
service that scheduler. Buzzer output uses ESP32 LEDC PWM; the default volume
is 30% of the 50%-duty maximum. Volume can be set with `M712` or in application
code with `processor->setSoundVolume(0-100)`.

Send these commands over serial (`S` is the sound selection or setting):

| Command | Action |
| --- | --- |
| `M700 S1` | Play the startup melody |
| `M700 S2` | Play the completion melody |
| `M700 S3` | Play a single tone |
| `M700 S4` | Play a random tone |
| `M700 S5` | Play the crash melody |
| `M700 S6` | Play the custom theme |
| `M700 S7` | Play the death melody |
| `M710 S0` / `M710 S1` | Disable / enable all sound |
| `M711 S0` / `M711 S1` | Disable / enable step sounds |
| `M712 S0`–`M712 S100` | Set sound volume (percent) |

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
(`M700 S6`) non-blockingly while the LED stays deep purple for 2 seconds, then pink
for 2 seconds, repeating in time with the melody. Any incoming serial data
stops the standby melody immediately and exits standby. The crash pattern (red
flash) is available in `RgbStatus`, but automatic crash detection is not
implemented.

| Command | Action |
| --- | --- |
| `M720 S0` | Turn the RGB LED off. |
| `M720 S1` | Turn the RGB LED on and resume its current status pattern. |
| `M721 S0`–`M721 S255` | Set RGB brightness from off to maximum. The default after reset is 128. |

Like the sound and movement schedulers, RGB patterns are advanced by periodic
`update()` calls rather than delays. LED frame transmission itself takes a
brief synchronous transfer to the addressable LED. Brightness changes apply
immediately and are not saved across resets.
