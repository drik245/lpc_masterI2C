// ESP32-WROOM Melody Player — Transistor-Amplified Piezo
// GPIO controls a transistor that switches external battery voltage
// through the piezo for much louder output
//
// Wiring:
//   GPIO25 -> 1kΩ resistor -> transistor Base
//   Battery+ -> piezo terminal 1
//   Piezo terminal 2 -> transistor Collector
//   Transistor Emitter -> GND
//   Battery- -> ESP32 GND (shared ground!)

#include "driver/gpio.h"

#define BUZZER_PIN 25

// Note frequencies (Hz)
#define C4  262
#define D4  294
#define E4  330
#define F4  349
#define G4  392
#define A4  440
#define B4  494
#define C5  523

// Melody & timing
static const uint16_t melody[] = { C4, D4, E4, F4, G4, A4, B4, C5 };
#define MELODY_LEN  (sizeof(melody) / sizeof(melody[0]))
#define NOTE_MS     400
#define STOP_MS     1000
#define LOOP_PAUSE  2000

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  gpio_set_drive_capability((gpio_num_t)BUZZER_PIN, GPIO_DRIVE_CAP_3);
}

void loop() {
  for (uint8_t i = 0; i < MELODY_LEN; i++) {
    tone(BUZZER_PIN, melody[i]);
    delay(NOTE_MS);
    noTone(BUZZER_PIN);
    delay(STOP_MS);
  }
  delay(LOOP_PAUSE);
}
