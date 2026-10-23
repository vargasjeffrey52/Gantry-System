# Gantry-System
This project is intended to control an x-y gantry system using a microcontroller.

## Sound commands

The passive buzzer is connected to GPIO 4. Sound playback is scheduled by
`SoundManager::update()` without delays, and the main loop and motion stepping
service that scheduler.

Send these commands over serial (`S` is the sound selection or setting):

| Command | Action |
| --- | --- |
| `M700 S1` | Play the startup melody |
| `M700 S2` | Play the completion melody |
| `M700 S3` | Play a single tone |
| `M700 S4` | Play a random tone |
| `M700 S5` | Play the crash melody |
| `M710 S0` / `M710 S1` | Disable / enable all sound |
| `M711 S0` / `M711 S1` | Disable / enable step sounds |

The startup melody plays once during boot, and the completion melody plays
after a supported movement command finishes. Step sounds are off by default
and are suppressed while a melody or other tone is playing. When enabled,
their pitch follows the movement speed, rising during acceleration and
falling during deceleration and after the move. `M700 S5` requests
the crash melody manually; automatic crash playback requires crash detection
hardware and logic, which are not currently implemented.
