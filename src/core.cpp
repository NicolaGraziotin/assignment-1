#include <Arduino.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include "core.h"
#include "kernel.h"
#include "config.h"
#include "input.h"
#include "output.h"

#define INTRO_TIMEOUT 10000 // ms waiting for B1+B2 before deep sleep
#define FINAL_TIMEOUT 10000
#define MAX_NUMBER 3

static int roundNumber = 1;
static int scoreP1 = 0;
static int scoreP2 = 0;
static int counterPlayer1 = 0;
static int counterPlayer2 = 0;
static int lastShownLevel = -1;
static int remainingTimeMs = 0;

void initCore(){
  Serial.begin(9600);
  srand(analogRead(A1)); // seed random number generator with noise from unconnected analog pin
}

void resetScores(){
  scoreP1 = 0;
  scoreP2 = 0;
}

int getScoreP1(){ return scoreP1; }
int getScoreP2(){ return scoreP2; }

void startTimer(int durationMs) {
    remainingTimeMs = durationMs;
}

void intro(){
  bool firstTime = isJustEnteredInState();
  if (firstTime){
    Serial.println("Welcome to the React! Game. Press Buttons to Start");
    resetInput();
    turnOffAllLeds(); // greens off, reds driven by pulse below
    updateDifficultyLevel();
    lastShownLevel = getDifficultyLevel();
  }

  // single-line rotating message
  updateWelcomeScroll(firstTime);

  // difficulty selected with Pot
  if (firstTime){
    showDifficultyLevel(lastShownLevel);
  }
  updateDifficultyLevel();
  int level = getDifficultyLevel();
  if (level != lastShownLevel){
    lastShownLevel = level;
    showDifficultyLevel(level);
  }

  // L2 and L3 fading in/out in counterphase, greens stay off
  pulseRedLedsInCounterphase(getCurrentTimeInState());

  // start when BOTH buttons have been pressed
  if (areBothButtonsPressed()){
    turnOffAllLeds();
    resetScores();
    resetInput();
    showGoMessage();
    Serial.println("Go!");
    changeState(STAGE1_STATE);
    return;
  }

  // 10 s without both buttons -> deep sleep
  if (getCurrentTimeInState() > INTRO_TIMEOUT){
    changeState(DEEP_SLEEP_STATE);
  }
}

void stage1(){
  bool firstTime = isJustEnteredInState();
  if (firstTime){
    showGoMessage();
    Serial.println("Game started.");
    int waitTime = random(0, MAX_WAIT + 1); // 0...MAX_WAIT ms
    startTimer(waitTime);
  }
  showPlayerScores(scoreP1, scoreP2);

  if (isButtonPressed(PLAYER_1_ID)) {
    scoreP1 = max(0, scoreP1 - 1);
    Serial.println("Player 1 pressed too early!");
    resetInput();
  }
  if (isButtonPressed(PLAYER_2_ID)) {
    scoreP2 = max(0, scoreP2 - 1);
    Serial.println("Player 2 pressed too early!");
    resetInput();
  }

  if (hasSystemTickPassed()) {
    if (remainingTimeMs > 0) {
      remainingTimeMs -= 100;
      Serial.print("Tempo rimanente: ");
      Serial.println(remainingTimeMs);
    }

    if (remainingTimeMs <= 0) {
      changeState(STAGE2_STATE);
    }
    // TODO: implement the game logic for stage 1, including checking button presses and updating scores
  }
}

void stage2(){
  bool firstTime = isJustEnteredInState();
  int nextNumber;
  if (firstTime){
    showRoundNumber(roundNumber);
    roundNumber++;
    nextNumber = random(1, MAX_NUMBER + 1); // 1...MAX_NUMBER
    Serial.println("Next number: " + String(nextNumber));
    showNextNumber(nextNumber);
    int diffTimer = max(0, T1-(F_FACTOR*getDifficultyLevel()*roundNumber));
    startTimer(diffTimer);
    counterPlayer1 = 0;
    counterPlayer2 = 0;
  }
  if (isButtonPressed(PLAYER_1_ID)) {
    counterPlayer1++;
    Serial.println("Player 1 pressed. Counter: " + String(counterPlayer1));
    resetInput();
  }
  if (isButtonPressed(PLAYER_2_ID)) {
    counterPlayer2++;
    Serial.println("Player 2 pressed. Counter: " + String(counterPlayer2));
    resetInput();
  }

  if (counterPlayer1 == nextNumber) {
    scoreP1++;
    changeState(STAGE1_STATE);
  } else if (counterPlayer1 > nextNumber) {
    scoreP1 = max(0, scoreP1 - 1);
    changeState(STAGE1_STATE);
  }
  if (counterPlayer2 == nextNumber) {
    scoreP2++;
    changeState(STAGE1_STATE);
  } else if (counterPlayer2 > nextNumber) {
    scoreP2 = max(0, scoreP2 - 1);
    changeState(STAGE1_STATE);
  }

  if (hasSystemTickPassed()) {
    if (remainingTimeMs > 0) {
      remainingTimeMs -= 100;
      Serial.print("Tempo rimanente: ");
      Serial.println(remainingTimeMs);
    }

    if (remainingTimeMs <= 0) {
      changeState(FINAL_STATE);
    }
  }
}

void deepSleep(){
  bool firstTime = isJustEnteredInState();
  if (!firstTime){
    return;
  }
  Serial.println("Going to sleep. Press B1 or B2 to wake up.");
  Serial.flush();

  turnOffAllLeds();
  sleepDisplay();
  resetInput();

  // game CHANGE interrupts off, pin-change wake-up on B1/B2
  inputPrepareForDeepSleep();

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu(); // ---- sleeps here, resumes here on B1/B2 ----

  sleep_disable();
  inputResumeFromDeepSleep();
  wakeDisplay();
  Serial.println("Woke up, back to intro.");
  changeState(INTRO_STATE);
}

void finalize(){
  bool firstTime = isJustEnteredInState();
  int winner;
  if (firstTime){
    winner = 0;
    if (scoreP1 > scoreP2) {
      winner = 1;
    } else if (scoreP2 > scoreP1) {
      winner = 2;
    }
    Serial.println("Final score - Player 1: " + String(scoreP1) + ", Player 2: " + String(scoreP2));
    if (winner == 1) {
      Serial.println("Player 1 wins!");
    } else if (winner == 2) {
      Serial.println("Player 2 wins!");
    } else {
      Serial.println("It's a tie!");
    }
    startTimer(FINAL_TIMEOUT);
  }
  showEndGameMessage(firstTime, winner);

  if (hasSystemTickPassed()) {
    if (remainingTimeMs > 0) {
      remainingTimeMs -= 100;
      Serial.print("Tempo rimanente: ");
      Serial.println(remainingTimeMs);
    }

    if (remainingTimeMs <= 0) {
      changeState(INTRO_STATE);
    }
  }
}
