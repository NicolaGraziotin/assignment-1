#include <Arduino.h>
#include <avr/sleep.h>
#include "core.h"
#include "kernel.h"
#include "config.h"
#include "input.h"
#include "output.h"

#define INTRO_TIMEOUT 10000 // ms waiting for B1+B2 before deep sleep

static int scoreP1 = 0;
static int scoreP2 = 0;
static int lastShownLevel = -1;

void initCore(){
  Serial.begin(9600);
}

void resetScores(){
  scoreP1 = 0;
  scoreP2 = 0;
}

int getScoreP1(){ return scoreP1; }
int getScoreP2(){ return scoreP2; }

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
  // TODO: rounds logic. For now keep the Go! message visible.
  bool firstTime = isJustEnteredInState();
  if (firstTime){
    showGoMessage();
    Serial.println("Game started.");
  }
  showPlayerScores(scoreP1, scoreP2);
}

void stage2(){}

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

void finalize(){}
