#include <Arduino.h>
#include "core.h"
#include "kernel.h"
#include "input.h"
#include "output.h"

#define MAX_TIME_IN_INTRO_STATE 10000 // time in milliseconds to wait in the intro state before transitioning to deep sleep
#define MAX_TIME_IN_STAGE2_STATE 10000

void initCore(){
  Serial.begin(9600);
}

void intro(){
  bool firstTime = isJustEnteredInState();
  if (firstTime){
    Serial.println("Welcome to the React! Game. Press Buttons to Start");
  }
  updateWelcomeScroll(firstTime);
  pulseRedLedsInCounterphase(getCurrentTimeInState());
  
  // int dt = getCurrentTimeInState();
  // if (dt > MAX_TIME_IN_INTRO_STATE){
  //   changeState(DEEP_SLEEP_STATE);
  // }
}

void stage1(){}
void stage2(){}
void deepSleep(){}
void finalize(){}
