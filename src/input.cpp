#include <Arduino.h>
#include <EnableInterrupt.h>
#include "config.h"

// #define __DEBUG__

#define BOUNCING_TIME 100
#define NUM_LEVELS 4
#define EASY 1
#define MEDIUM 2
#define HARD 3
#define EXPERT 4

uint8_t inputPins[NUM_BUTTONS] = {BUTTON_PLAYER_1, BUTTON_PLAYER_2};
bool buttonPressed[NUM_BUTTONS] = {false, false};

uint8_t difficultyLevel[NUM_LEVELS] = {EASY, MEDIUM, HARD, EXPERT};
int potentiometerValue = 0;
int lastPotentiometerValue = 0;

/* used for debouncing */
long lastButtonPressedTimestamps[NUM_BUTTONS];

void buttonHandler(int i);
void buttonHandler0(){ buttonHandler(0); }
void buttonHandler1(){ buttonHandler(1); }

void (*buttonHandlers[NUM_BUTTONS])() = { buttonHandler0, buttonHandler1 };

void buttonHandler(int i){
  long ts = millis();
  if (ts - lastButtonPressedTimestamps[i] > BOUNCING_TIME){
    lastButtonPressedTimestamps[i] = ts;
    int status = digitalRead(inputPins[i]);
    if (status == HIGH && !buttonPressed[i]) { 
        buttonPressed[i] = true;
    }
  }
}

void initInput(){
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(inputPins[i], INPUT);  
    enableInterrupt(inputPins[i], buttonHandlers[i], CHANGE);
  }
}

void resetInput(){
  long ts = millis();
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttonPressed[i] = false;      
    lastButtonPressedTimestamps[i] = ts;
  }
}
bool isButtonPressed(int buttonIndex){
  return buttonPressed[buttonIndex];
}

bool hasBeenPressed(int index){
  return buttonPressed[index]; 
}

void setDifficultyLevel(int level){
  if (level >= 1 && level <= NUM_LEVELS){
    difficultyLevel[level - 1] = level;
  }
}

void setPotentiometerValue(int rawValue){
  potentiometerValue = map(rawValue, 0, 1023, 1, NUM_LEVELS);
  setDifficultyLevel(potentiometerValue);
}

void potentiometerHandler(){
  int rawValue = analogRead(POTENTIOMETER);
  if (rawValue != lastPotentiometerValue){
    lastPotentiometerValue = rawValue;
    setPotentiometerValue(rawValue);
  }
}

void getDifficultyLevel(int &level){
  level = potentiometerValue;
}