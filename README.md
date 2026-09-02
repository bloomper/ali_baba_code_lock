# Code Lock

A three-button code lock built with Arduino, housed in a wooden box, that plays the Jeopardy "Think" theme when you enter the right sequence — and a sad trombone when you don't.

Also features a Konami-code easter egg and a "self-destruct" sequence that triggers after too many failed attempts.

## Features

- **Three-button secret code** — enter the correct sequence to trigger the win melody
- **Konami easter egg** — a longer hidden sequence plays a special fanfare
- **Self-destruct mode** — three wrong codes within 30 seconds triggers a countdown followed by an explosion sound
- **Input timeout** — half-entered sequences reset automatically after 5 seconds
- **Hybrid audio** — plays high-quality MP3s from an SD card via the Adafruit Music Maker Shield on real hardware, and gracefully falls back to `tone()` beeps on a buzzer when the shield isn't detected (perfect for simulation)
- **Wokwi-ready** — the same sketch runs in the Wokwi browser simulator without modification

## Hardware

| Component | Purpose |
|---|---|
| Arduino Uno R3 | Main controller |
| Adafruit Music Maker Shield (VS1053, Adafruit #1790) | MP3 playback |
| MicroSD card | Stores audio files |
| 3× microswitches with lever arm | Button contacts |
| 3× large wooden buttons | User-facing buttons |
| Springs | Balance the buttons |
| Latching pushbutton | Power on/off |
| 3.5mm panel jack | Line-out to external speaker |
| 9V battery + holder with DC plug | Power supply |
| Active speaker with 3.5mm input | Audio output |
| Wooden box | Enclosure |

### Wiring

```
Buttons:  Each microswitch COM → Arduino GND
          Each microswitch NO  → Arduino A0, A1, A2

Audio:    Music Maker headphone out → 3.5mm panel jack (inside box)
          Panel jack (outside box)  → external active speaker

Power:    Battery (+) → power switch → DC jack (+) → Arduino
          Battery (−) → DC jack (−) → Arduino
```

Fallback buzzer (Wokwi / diagnostic): pin 8 → GND.

## Software Setup

### Required Library

Install via Arduino IDE Library Manager:

- **Adafruit VS1053 Library**

### SD Card Contents

Format the SD card as FAT16 or FAT32 and place these files in the root (filenames must be 8.3 format):

| File | Sound |
|---|---|
| `win.mp3` | Jeopardy "Think" theme |
| `fail.mp3` | Sad trombone |
| `konami.mp3` | Fanfare / achievement sound |
| `btn0.mp3` | Sound for button 0 (~50–100 ms) — swap the file to change it |
| `btn1.mp3` | Sound for button 1 (~50–100 ms) — swap the file to change it |
| `btn2.mp3` | Sound for button 2 (~50–100 ms) — swap the file to change it |
| `tick.mp3` | Ticking sound for self-destruct countdown (~1 s) |
| `boom.mp3` | Explosion |

Free sound sources: [freesound.org](https://freesound.org), [pixabay.com/sound-effects](https://pixabay.com/sound-effects).

### Uploading

Open `sketch.ino` in the Arduino IDE, select **Arduino Uno** as the board, choose your port, and upload.

## Simulation with Wokwi

You can run this project entirely in your browser using [Wokwi](https://wokwi.com) — no hardware needed.

1. Create a new Arduino Uno project at https://wokwi.com/projects/new/arduino-uno
2. Paste the contents of `code_lock_hybrid.ino` into the sketch tab
3. Paste `diagram.json` into the diagram.json tab
4. Add `libraries.txt` (via Library Manager or by creating the file) with:
   ```
   Adafruit VS1053 Library
   ```
5. Click the green ▶ play button and click the buttons with your mouse

The sketch will detect that no VS1053 chip is present and fall back to buzzer output — the logic runs identically to the hardware version.

## Usage

Once powered on, the Arduino boots up with two short beeps.

### Correct Code

Press buttons in sequence: **0 → 1 → 2** (red → green → blue in the simulator).

You'll hear the Jeopardy "Think" theme.

### Konami Code

Press: **0, 0, 1, 1, 2, 2** (red, red, green, green, blue, blue).

You'll hear a fanfare — you found the easter egg!

### Self-Destruct

Enter three wrong codes within 30 seconds. The Arduino counts down from 5 with ticking sounds, then plays an explosion. The system resets after that.

### Timeout

If you pause for more than 5 seconds mid-sequence, the input resets. A low tone signals the timeout.

## Configuration

All configurable constants are at the top of the sketch:

```cpp
const int SECRET_CODE[3] = {0, 1, 2};                   // The normal code
const int KONAMI_CODE[]  = {0, 0, 1, 1, 2, 2};          // The easter egg
const int FAIL_LIMIT     = 3;                            // Failures before self-destruct
const unsigned long FAIL_WINDOW   = 30000;               // Fail window (ms)
const int COUNTDOWN_FROM = 5;                            // Countdown starting value
const unsigned long INPUT_TIMEOUT = 5000;                // Sequence timeout (ms)
```

Change any of these and re-upload. The sketch validates lengths automatically via `sizeof()`.

## Project Structure

```
.
├── code_lock_hybrid.ino    # Main sketch (hybrid: real hardware + Wokwi)
├── diagram.json            # Wokwi circuit definition
├── libraries.txt           # Wokwi library list
└── README.md               # This file
```

## How the Hybrid Mode Works

At startup, `setup()` calls `musicPlayer.begin()`. If it returns true, a VS1053 chip is present and the sketch uses the Music Maker Shield for all audio. If it returns false — because you're running in Wokwi, or the shield isn't connected, or the SD card is missing — the sketch prints a diagnostic and falls back to `tone()` on the buzzer pin.

Every sound function checks the `useMusicMaker` flag internally:

```cpp
void playSound(const char* filename, int melody[], int durations[], int length) {
  if (useMusicMaker) {
    musicPlayer.playFullFile(filename);
  } else {
    // play the melody with tone()
  }
}
```

Benefits:

- **One sketch, two targets** — no separate simulator and hardware branches to maintain
- **Graceful degradation** — a loose shield connection or missing SD card doesn't brick the device
- **Faster iteration** — develop and debug logic in Wokwi with instant reload, deploy the same code
- **Self-diagnosing** — Serial output reveals which mode is active, making hardware issues obvious

## Enclosure

The physical build uses a plywood box with three large (~20 cm diameter) plywood buttons on the top face. Each button sits *inside* the box and pokes up through a slightly smaller hole in the lid, so the button's rim rests against the underside of the lid.

When pressed, the button slides down a few millimeters and activates a microswitch mounted centrally beneath it. Two springs on either side of the switch balance the button so it presses evenly regardless of where on the surface the user pushes.

- **Buttons:** ~20 cm diameter, ~15 mm thick plywood discs
- **Holes in lid:** ~12 cm diameter (smaller than buttons)
- **Vertical travel:** 3–5 mm
- **Springs:** 2 per button, ~100 g each (light — plywood buttons weigh only ~250 g)

The box also has:
- A **latching pushbutton** on the side for power on/off
- A **3.5mm jack** on the side for connecting the external speaker
- A **battery hatch** on the underside for easy 9V battery replacement without opening the box

## Credits

- **Melodies:** Jeopardy "Think" theme by Merv Griffin (used for personal/hobby project)
- **Adafruit VS1053 Library:** [github.com/adafruit/Adafruit_VS1053_Library](https://github.com/adafruit/Adafruit_VS1053_Library)
- **Simulator:** [Wokwi](https://wokwi.com)

