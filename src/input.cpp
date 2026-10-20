#include <Arduino.h>
#include <TimerOne.h>
#include "config.h"

// #define __DEBUG__

// Game buttons use the Uno hardware external interrupts (pins 2, 3).
// Deep-sleep wake-up uses pin-change interrupts instead: with the
// pull-down wiring (idle LOW) a LOW-level wake-up would fire at once,
// while PCINT wakes on any toggle and works in power-down.

#define BOUNCING_TIME 200
#define NUM_LEVELS 4


static unsigned long stageTimerStart = 0;
static unsigned long stageTimerDuration = 0;
static bool stageTimerRunning = false;


uint8_t inputPins[NUM_BUTTONS] = {BUTTON_PLAYER_1, BUTTON_PLAYER_2};
volatile bool buttonPressed[NUM_BUTTONS] = {false, false};
static volatile int clickCount[NUM_BUTTONS] = {0, 0};

/* difficulty level 1..4 selected with Pot before the game starts */
static int currentDifficultyLevel = 1;

/* used for debouncing */
static long lastButtonPressedTimestamps[NUM_BUTTONS] = {0, 0};

void buttonHandler(int i);
void buttonISR0(){ buttonHandler(0); }
void buttonISR1(){ buttonHandler(1); }
void (*buttonISRs[NUM_BUTTONS])() = { buttonISR0, buttonISR1 };
void resetInput();


void buttonHandler(int i) {
    unsigned long ts = millis();

    if (ts - lastButtonPressedTimestamps[i] < BOUNCING_TIME) {
        return;
    }

    int status = digitalRead(inputPins[i]);

    // Accetta soltanto il fronte di salita.
    if (status == HIGH) {
        lastButtonPressedTimestamps[i] = ts;
        buttonPressed[i] = true;
        clickCount[i]++;
    }
}


void initInput(){
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(inputPins[i], INPUT);
    attachInterrupt(digitalPinToInterrupt(inputPins[i]), buttonISRs[i], CHANGE);
  }
  resetInput();
}

void resetInput(){
  long ts = millis();
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttonPressed[i] = false;
    lastButtonPressedTimestamps[i] = ts;
    clickCount[i] = 0;
  }
}

bool isButtonPressed(int buttonIndex){
  if (buttonIndex < 0 || buttonIndex >= NUM_BUTTONS) {
    return false;
  }
  return buttonPressed[buttonIndex];
}

bool hasBeenPressed(int index){
  return isButtonPressed(index);
}

bool areBothButtonsPressed(){
  return buttonPressed[0] && buttonPressed[1];
}

void updateDifficultyLevel(){
  int rawValue = analogRead(POTENTIOMETER);
  int level = map(rawValue, 0, 1000, 1, NUM_LEVELS); //POTENZIOMETRO DIFFETTOSO NON LEGGE 1023 MA SOLO FINO A 1000, QUINDI HO MODIFICATO IL MAP
  // int level = map(rawValue, 0, 1023, 1, NUM_LEVELS);
  level = constrain(level, 1, NUM_LEVELS);
  currentDifficultyLevel = level;
}

int getDifficultyLevel(){
  return currentDifficultyLevel;
}

// Call before sleep_cpu(): game ISRs off, pin-change wake-up on D2/D3.
// D2 = PCINT18, D3 = PCINT19, both in the PCIE2 group.
ISR(PCINT2_vect){ /* empty: waking the MCU is enough */ }

void inputPrepareForDeepSleep(){
  for (int i = 0; i < NUM_BUTTONS; i++) {
    detachInterrupt(digitalPinToInterrupt(inputPins[i]));
  }
  PCIFR |= (1 << PCIF2); // clear any pending pin-change interrupt
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  PCICR |= (1 << PCIE2);
}

// Call after wake: pin-change wake-up off, game ISRs back on.
void inputResumeFromDeepSleep(){
  PCICR &= ~(1 << PCIE2);
  PCMSK2 &= ~((1 << PCINT18) | (1 << PCINT19));
  for (int i = 0; i < NUM_BUTTONS; i++) {
    attachInterrupt(digitalPinToInterrupt(inputPins[i]), buttonISRs[i], CHANGE);
  }
  resetInput();
}

int getPlayerClicks(int playerIndex){
  if (playerIndex < 0 || playerIndex >= NUM_BUTTONS) {
    return 0;
  }
  noInterrupts();
  int count = clickCount[playerIndex];
  interrupts();
  return count;
}

void startStageTimer(unsigned long durationMs) {
    stageTimerStart = millis();
    stageTimerDuration = durationMs;
    stageTimerRunning = true;
}

void stopStageTimer() {
    stageTimerRunning = false;
}

bool isStageTimerExpired() {
    if (!stageTimerRunning) {
        return false;
    }

    return (unsigned long)(millis() - stageTimerStart)
           >= stageTimerDuration;
}

void resetStageTimerFlag() {
    stageTimerRunning = false;
}