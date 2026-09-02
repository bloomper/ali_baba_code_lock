/*
  Code Lock - Music Maker with buzzer fallback
  ----------------------------------------------------
  Same code works both in the Wokwi simulator AND on real hardware.

  - If Music Maker Shield (VS1053) is found: plays mp3 files from SD card
  - If not (e.g. in Wokwi): falls back to tone() on a buzzer

  HARDWARE (real):
    - Arduino Uno + MP3 player shield (Adafruit 1790)
    - 3 pushbuttons between A0/A1/A2 and GND
    - Speaker via 3.5mm

  WOKWI WIRING:
    - 3 pushbuttons between A0/A1/A2 and GND
    - Buzzer on pin 8

  SD CARD FILES (only relevant for real hardware):
    win.mp3, fail.mp3, konami.mp3, tick.mp3, boom.mp3
    btn0.mp3, btn1.mp3, btn2.mp3  <- one per button, swap them to change the sounds
*/

#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

// ---------- Music Maker Shield pins ----------
#define SHIELD_RESET  -1
#define SHIELD_CS      7
#define SHIELD_DCS     6
#define CARDCS         4
#define DREQ           3

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

// ---------- Fallback buzzer ----------
const int BUZZER_PIN = 8;

// ---------- Mode ----------
bool useMusicMaker = false;  // set to true if VS1053 is detected

// ---------- Buttons ----------
const int BUTTON_PINS[3] = {A0, A1, A2};

// ---------- Per-button sounds ----------
// Real hardware: replace these files on the SD card to change each button's sound.
const char* BUTTON_FILES[3] = {"btn0.mp3", "btn1.mp3", "btn2.mp3"};
// Wokwi/buzzer fallback: one pitch per button so they are easy to tell apart.
const int BUTTON_TONES[3]   = {1000, 1200, 1400};
const int BUTTON_TONE_MS    = 40;

// On the third press the win/fail sound follows immediately. The button sound is
// started asynchronously on the shield, so it has to be waited for or it never
// gets heard. The timeout is a safety net for a missing or corrupt file.
const unsigned long BUTTON_SOUND_MAX_WAIT = 2000;
const int           SOUND_GAP_MS          = 120;

// ---------- Secret codes ----------
const int SECRET_CODE[3] = {0, 1, 2};
const int KONAMI_CODE[] = {0, 0, 1, 1, 2, 2};
const int KONAMI_LENGTH = sizeof(KONAMI_CODE) / sizeof(int);

// ---------- Self-destruct ----------
const int      FAIL_LIMIT       = 3;
const unsigned long FAIL_WINDOW = 30000;
const int      COUNTDOWN_FROM   = 5;
const unsigned long INPUT_TIMEOUT = 5000;

// ---------- Note frequencies (for buzzer fallback) ----------
#define NOTE_C4  262
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_Gb4 370
#define NOTE_Eb4 311
#define REST     0

int winMelody[]    = {NOTE_C5, NOTE_F5, NOTE_C5, NOTE_F4, NOTE_C5, NOTE_F5, NOTE_C5, REST,
                       NOTE_C5, NOTE_F5, NOTE_C5, NOTE_F5, NOTE_D5, NOTE_C5, NOTE_A4, NOTE_F4};
int winDurations[] = {4,4,4,4, 4,4,2,4, 4,4,4,4, 4,4,4,2};
int winLength      = sizeof(winMelody) / sizeof(int);

int failMelody[]    = {NOTE_C5, NOTE_B4, NOTE_Gb4, NOTE_Eb4};
int failDurations[] = {4, 4, 4, 2};
int failLength      = sizeof(failMelody) / sizeof(int);

int konamiMelody[]    = {NOTE_E5, NOTE_E5, REST, NOTE_E5, REST, NOTE_C5, NOTE_E5,
                          NOTE_G5, REST, REST, NOTE_G4};
int konamiDurations[] = {8,8,8,8,8,8,4,4,4,4,4};
int konamiLength      = sizeof(konamiMelody) / sizeof(int);

// ---------- State ----------
const int MAX_SEQ = 10;
int sequence[MAX_SEQ];
int pressCount = 0;
unsigned long lastPressTime = 0;

bool lastButtonState[3] = {HIGH, HIGH, HIGH};

int           failCount = 0;
unsigned long firstFailTime = 0;

void setup() {
  Serial.begin(9600);
  Serial.println(F("Code Lock starting..."));

  for (int i = 0; i < 3; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }
  pinMode(BUZZER_PIN, OUTPUT);

  // Try to find the Music Maker Shield
  if (musicPlayer.begin()) {
    Serial.println(F("VS1053 found!"));
    if (SD.begin(CARDCS)) {
      Serial.println(F("SD card OK."));
      musicPlayer.setVolume(20, 20);
      musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);
      useMusicMaker = true;
      Serial.println(F("Mode: Music Maker Shield (mp3)"));
    } else {
      Serial.println(F("No SD card - falling back to buzzer"));
    }
  } else {
    Serial.println(F("No VS1053 detected - falling back to buzzer"));
    Serial.println(F("Mode: Buzzer (tone)"));
  }

  // Startup signal
  playButtonSound(0);
  delay(150);
  playButtonSound(2);

  Serial.println(F("Ready. Press the buttons..."));
  Serial.println(F("Normal code: 0, 1, 2"));
  Serial.println(F("Konami: 0,0,1,1,2,2"));
}

void loop() {
  if (pressCount > 0 && (millis() - lastPressTime) > INPUT_TIMEOUT) {
    Serial.println(F("Timeout - sequence reset."));
    if (!useMusicMaker) {
      tone(BUZZER_PIN, 300, 200);
      delay(250);
    }
    pressCount = 0;
  }

  for (int i = 0; i < 3; i++) {
    bool currentState = digitalRead(BUTTON_PINS[i]);

    if (lastButtonState[i] == HIGH && currentState == LOW) {
      delay(50);
      if (digitalRead(BUTTON_PINS[i]) == LOW) {
        registerPress(i);
      }
    }
    lastButtonState[i] = currentState;
  }
}

void registerPress(int buttonIndex) {
  sequence[pressCount] = buttonIndex;
  pressCount++;
  lastPressTime = millis();

  Serial.print(F("Press "));
  Serial.print(pressCount);
  Serial.print(F(": button "));
  Serial.println(buttonIndex);

  playButtonSound(buttonIndex);

  if (pressCount == KONAMI_LENGTH && checkKonami()) {
    Serial.println(F("*** KONAMI CODE! ***"));
    playSound("konami.mp3", konamiMelody, konamiDurations, konamiLength);
    resetAll();
    return;
  }

  if (pressCount == 3) {
    if (checkSecret()) {
      Serial.println(F("CORRECT CODE!"));
      playSound("win.mp3", winMelody, winDurations, winLength);
      resetAll();
      return;
    } else {
      if (!konamiStillPossible()) handleFail();
    }
  }

  if (pressCount > 3 && !konamiStillPossible()) {
    handleFail();
    return;
  }

  if (pressCount >= KONAMI_LENGTH) handleFail();
}

void handleFail() {
  Serial.println(F("WRONG CODE!"));
  playSound("fail.mp3", failMelody, failDurations, failLength);

  unsigned long now = millis();
  if (failCount == 0 || (now - firstFailTime) > FAIL_WINDOW) {
    failCount = 1;
    firstFailTime = now;
  } else {
    failCount++;
  }

  Serial.print(F("Fail count: "));
  Serial.print(failCount);
  Serial.print(F(" of "));
  Serial.println(FAIL_LIMIT);

  if (failCount >= FAIL_LIMIT) selfDestruct();

  pressCount = 0;
}

void selfDestruct() {
  Serial.println(F("*** SELF-DESTRUCT ***"));
  delay(500);

  for (int i = COUNTDOWN_FROM; i >= 1; i--) {
    Serial.print(F("Countdown: "));
    Serial.println(i);
    if (useMusicMaker) {
      musicPlayer.playFullFile("tick.mp3");
    } else {
      tone(BUZZER_PIN, 800, 200);
      delay(1000);
    }
  }

  Serial.println(F("BOOM!"));
  if (useMusicMaker) {
    musicPlayer.playFullFile("boom.mp3");
  } else {
    for (int f = 800; f > 50; f -= 10) {
      tone(BUZZER_PIN, f + random(-50, 50));
      delay(15);
    }
    noTone(BUZZER_PIN);
  }

  resetAll();
  Serial.println(F("Reset. Try again..."));
}

// ---------- Sound functions ----------

void playButtonSound(int buttonIndex) {
  if (buttonIndex < 0 || buttonIndex > 2) return;

  if (useMusicMaker) {
    // Returns straight away - the file keeps playing in the background so that
    // presses stay responsive. waitForButtonSound() lets it finish.
    if (musicPlayer.playingMusic) musicPlayer.stopPlaying();
    musicPlayer.startPlayingFile(BUTTON_FILES[buttonIndex]);
  } else {
    tone(BUZZER_PIN, BUTTON_TONES[buttonIndex], BUTTON_TONE_MS);
    delay(BUTTON_TONE_MS * 2);
  }
}

// Let a button sound that is still playing finish before the next sound starts.
void waitForButtonSound() {
  if (!useMusicMaker) return;

  unsigned long start = millis();
  while (musicPlayer.playingMusic && (millis() - start) < BUTTON_SOUND_MAX_WAIT) {
    delay(5);
  }
}

void playSound(const char* filename, int melody[], int durations[], int length) {
  waitForButtonSound();  // the third press must be heard before win/fail
  delay(SOUND_GAP_MS);   // short silence so the two sounds stay distinct

  if (useMusicMaker) {
    if (musicPlayer.playingMusic) musicPlayer.stopPlaying();
    musicPlayer.playFullFile(filename);
  } else {
    for (int i = 0; i < length; i++) {
      int noteDuration = 1000 / durations[i];
      if (melody[i] == REST) {
        noTone(BUZZER_PIN);
      } else {
        tone(BUZZER_PIN, melody[i], noteDuration);
      }
      delay(noteDuration * 1.30);
      noTone(BUZZER_PIN);
    }
  }
}

// ---------- Logic checks ----------

bool checkSecret() {
  for (int i = 0; i < 3; i++) {
    if (sequence[i] != SECRET_CODE[i]) return false;
  }
  return true;
}

bool checkKonami() {
  for (int i = 0; i < KONAMI_LENGTH; i++) {
    if (sequence[i] != KONAMI_CODE[i]) return false;
  }
  return true;
}

bool konamiStillPossible() {
  if (pressCount > KONAMI_LENGTH) return false;
  for (int i = 0; i < pressCount; i++) {
    if (sequence[i] != KONAMI_CODE[i]) return false;
  }
  return true;
}

void resetAll() {
  pressCount = 0;
  failCount = 0;
  firstFailTime = 0;
}
