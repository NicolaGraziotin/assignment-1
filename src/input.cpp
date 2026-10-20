#include <Arduino.h>
#include <avr/interrupt.h>
#include "config.h"
#include "input.h"

// Game buttons use the Uno hardware external interrupts (pins 2, 3).
// Deep-sleep wake-up uses pin-change interrupts instead: with the
// pull-down wiring (idle LOW) a LOW-level wake-up would fire at once,
// while PCINT wakes on any toggle and works in power-down.

#define BOUNCING_TIME 50
#define NUM_LEVELS 4
#define POT_SAMPLES 4

static const uint8_t inputPins[NUM_BUTTONS] = {BUTTON_PLAYER_1, BUTTON_PLAYER_2};

/* Number of presses not consumed yet by the game logic. */
static volatile uint8_t buttonPressCount[NUM_BUTTONS] = {0, 0};
/* Time of the last accepted press (high resolution, used to break a tie). */
static volatile unsigned long lastPressMicros[NUM_BUTTONS] = {0, 0};
/* Used for debouncing. */
static volatile unsigned long lastButtonPressedTimestamps[NUM_BUTTONS] = {0, 0};

/* Difficulty level 1..4 selected with the Pot before the game starts. */
static int currentDifficultyLevel = 1;

static void buttonHandler(int i);
static void buttonISR0() { buttonHandler(PLAYER_1_ID); }
static void buttonISR1() { buttonHandler(PLAYER_2_ID); }
static void (*buttonISRs[NUM_BUTTONS])() = {buttonISR0, buttonISR1};

static void buttonHandler(int i) {
  unsigned long now = millis();
  if (now - lastButtonPressedTimestamps[i] > BOUNCING_TIME) {
    lastButtonPressedTimestamps[i] = now;
    if (digitalRead(inputPins[i]) == HIGH) {
      // Count all presses: the game logic must also detect N+1 presses.
      if (buttonPressCount[i] < 255) {
        buttonPressCount[i]++;
      }
      lastPressMicros[i] = micros();
    }
  }
}

void initInput() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(inputPins[i], INPUT);
    attachInterrupt(digitalPinToInterrupt(inputPins[i]), buttonISRs[i], CHANGE);
  }
  resetInput();
}

void resetInput() {
  unsigned long now = millis();
  noInterrupts();
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttonPressCount[i] = 0;
    lastButtonPressedTimestamps[i] = now;
  }
  interrupts();
}

/* Consume and return all presses received on a button since the last call. */
int takeButtonPresses(int buttonIndex) {
  if (buttonIndex < 0 || buttonIndex >= NUM_BUTTONS) {
    return 0;
  }
  noInterrupts();
  int presses = buttonPressCount[buttonIndex];
  buttonPressCount[buttonIndex] = 0;
  interrupts();
  return presses;
}

bool isButtonPressed(int buttonIndex) {
  if (buttonIndex < 0 || buttonIndex >= NUM_BUTTONS) {
    return false;
  }
  return buttonPressCount[buttonIndex] > 0;
}

bool areBothButtonsPressed() {
  return buttonPressCount[PLAYER_1_ID] > 0 && buttonPressCount[PLAYER_2_ID] > 0;
}

unsigned long getLastPressTime(int buttonIndex) {
  if (buttonIndex < 0 || buttonIndex >= NUM_BUTTONS) {
    return 0;
  }
  noInterrupts();
  unsigned long timestamp = lastPressMicros[buttonIndex];
  interrupts();
  return timestamp;
}

void updateDifficultyLevel() {
  unsigned int sum = 0;
  for (int i = 0; i < POT_SAMPLES; i++) {
    sum += analogRead(POTENTIOMETER);
  }
  int rawValue = sum / POT_SAMPLES;

  // four equal ranges: 0..255 -> 1, 256..511 -> 2,
  // 512..767 -> 3, 768..1023 -> 4
  int level = 1 + (rawValue * NUM_LEVELS) / 1024;
  currentDifficultyLevel = constrain(level, 1, NUM_LEVELS);
}

int getDifficultyLevel() {
  return currentDifficultyLevel;
}

/* Call before sleep_cpu(): game ISRs off, pin-change wake-up on D2/D3.
 * D2 = PCINT18, D3 = PCINT19, both in the PCIE2 group. */
ISR(PCINT2_vect) { /* empty: waking the MCU is enough */ }

void inputPrepareForDeepSleep() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    detachInterrupt(digitalPinToInterrupt(inputPins[i]));
  }
  EIFR |= (1 << INTF0) | (1 << INTF1); // clear stale external-interrupt flags
  PCIFR |= (1 << PCIF2); // clear any pending pin-change interrupt
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  PCICR |= (1 << PCIE2);
}

/* Call after wake: pin-change wake-up off, game ISRs back on. */
void inputResumeFromDeepSleep() {
  PCICR &= ~(1 << PCIE2);
  PCMSK2 &= ~((1 << PCINT18) | (1 << PCINT19));
  PCIFR |= (1 << PCIF2);
  EIFR |= (1 << INTF0) | (1 << INTF1);
  for (int i = 0; i < NUM_BUTTONS; i++) {
    attachInterrupt(digitalPinToInterrupt(inputPins[i]), buttonISRs[i], CHANGE);
  }
  resetInput();
}
