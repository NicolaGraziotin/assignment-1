#include <Arduino.h>
#include "config.h"
#include "core.h"
#include "input.h"
#include "output.h"
#include "kernel.h"

void setup() {
  // put your setup code here, to run once:
  initCore();
  initInput();
  initOutput();
  changeState(INTRO_STATE);
}

void loop() {
  updateStateTime(); 
  switch (getCurrentState()) { 
  case INTRO_STATE:
    intro();
    break;
  case STAGE1_STATE:
    stage1();
    break;
  case STAGE2_STATE:
    stage2();
    break;
  case DEEP_SLEEP_STATE:
    deepSleep();
    break;
  case FINAL_STATE:
    finalize();
    break;
  }
}
