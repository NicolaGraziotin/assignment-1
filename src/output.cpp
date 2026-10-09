#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"

// #define __DEBUG__

#define LCD_COLS 20
#define LCD_ROWS 4
#define SCROLL_INTERVAL 300

uint8_t outputPins[NUM_LEDS] = {RED_LED_PLAYER_1, GREEN_LED_PLAYER_1, GREEN_LED_PLAYER_2, RED_LED_PLAYER_2};
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27,LCD_COLS,LCD_ROWS);

void initOutput(){
  for(int i = 0; i < NUM_LEDS; i++){
    pinMode(outputPins[i], OUTPUT);
  }
  lcd.init();
  lcd.backlight();
}

void setLedState(int ledIndex, bool state){
  if(ledIndex >= 0 && ledIndex < NUM_LEDS){
    digitalWrite(outputPins[ledIndex], state ? HIGH : LOW);
  }
}

void pulseRedLedsInCounterphase(int step){
  int brightnessL2 = (step % 256);
  int brightnessL3 = 255 - brightnessL2;
  analogWrite(RED_LED_PLAYER_1, brightnessL2);
  analogWrite(RED_LED_PLAYER_2, brightnessL3);
}

void printGameTitle(){
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print("React!");
}

void printDifficultyLevel(int level) {
  printGameTitle();
  lcd.setCursor(1, 2);
  lcd.print("Difficulty: ");
  lcd.setCursor(1, 3);
  switch (level) {
    case 1:
      lcd.print("1 (Easy)");
      break;
    case 2:
      lcd.print("2 (Medium)");
      break;
    case 3:
      lcd.print("3 (Hard)");
      break;
    case 4:
      lcd.print("4 (Expert)");
      break;
    default:
      lcd.print("Undefined");
      break;
  }
}

void updateWelcomeScroll(bool firstTime) {
  static String msg = "Welcome to the React! Game. Press Buttons to Start    ";
  static String doubled = msg + msg;
  static int pos = 0;
  static unsigned long last = 0;

  if (firstTime) {
    lcd.clear();
    pos = 0;
    last = millis();
  } else {
    if (millis() - last < SCROLL_INTERVAL) {
      return;
    }
    last = millis();
    pos = (pos + 1) % msg.length();
  }

  lcd.setCursor(0, 0);
  lcd.print(doubled.substring(pos, pos + LCD_COLS));
}