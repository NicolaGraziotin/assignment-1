#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "output.h"

#define LCD_COLS 20
#define LCD_ROWS 4
#define SCROLL_INTERVAL 300

/* Player-indexed led pins. */
static const uint8_t greenLedPins[NUM_BUTTONS] = {GREEN_LED_PLAYER_1, GREEN_LED_PLAYER_2};
static const uint8_t redLedPins[NUM_BUTTONS] = {RED_LED_PLAYER_1, RED_LED_PLAYER_2};

static LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);

/* State of the rotating LCD message. */
static String scrollBase;
static String scrollDoubled;
static int scrollPosition = 0;
static unsigned long scrollLastUpdate = 0;

/* Avoid rewriting the score line on every loop iteration. */
static int lastScoreP1 = 0;
static int lastScoreP2 = 0;
static bool scoreLineValid = false;

void initOutput() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(greenLedPins[i], OUTPUT);
    pinMode(redLedPins[i], OUTPUT);
  }
  turnOffAllLeds();
  lcd.init();
  lcd.backlight();
  clearDisplay();
}

void clearDisplay() {
  lcd.clear();
  scoreLineValid = false;
}

void setGreenLed(int player, bool state) {
  if (player >= 0 && player < NUM_BUTTONS) {
    digitalWrite(greenLedPins[player], state ? HIGH : LOW);
  }
}

void setRedLed(int player, bool state) {
  if (player >= 0 && player < NUM_BUTTONS) {
    digitalWrite(redLedPins[player], state ? HIGH : LOW);
  }
}

void setBothRedLeds(bool state) {
  setRedLed(PLAYER_1_ID, state);
  setRedLed(PLAYER_2_ID, state);
}

void turnOffAllLeds() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    digitalWrite(greenLedPins[i], LOW);
    digitalWrite(redLedPins[i], LOW);
  }
}

#define PULSE_STEP_MS 8
#define PULSE_HOLD_STEPS 128

void pulseRedLedsInCounterphase(unsigned long timeMs) {
  unsigned long period = 2UL * (256UL + PULSE_HOLD_STEPS);
  unsigned long step = (timeMs / PULSE_STEP_MS) % period;

  int brightnessL2;
  if (step < 256) {
    brightnessL2 = (int)step;
  } else if (step < 256 + PULSE_HOLD_STEPS) {
    brightnessL2 = 255;
  } else if (step < 512 + PULSE_HOLD_STEPS) {
    brightnessL2 = 255 - (int)(step - 256 - PULSE_HOLD_STEPS);
  } else {
    brightnessL2 = 0;
  }

  int brightnessL3 = 255 - brightnessL2;
  analogWrite(redLedPins[PLAYER_1_ID], brightnessL2);
  analogWrite(redLedPins[PLAYER_2_ID], brightnessL3);
}

static void beginScroll(const String &text) {
  scrollBase = text + "    ";
  scrollDoubled = scrollBase + scrollBase;
  scrollPosition = 0;
  scrollLastUpdate = millis();

  lcd.clear();
  scoreLineValid = false;
  lcd.setCursor(0, 0);
  lcd.print(scrollDoubled.substring(0, LCD_COLS));
}

static void updateScroll() {
  if (millis() - scrollLastUpdate < SCROLL_INTERVAL) {
    return;
  }
  scrollLastUpdate += SCROLL_INTERVAL;

  scrollPosition++;
  if (scrollPosition >= (int)scrollBase.length()) {
    scrollPosition = 0;
  }

  lcd.setCursor(0, 0);
  lcd.print(scrollDoubled.substring(scrollPosition, scrollPosition + LCD_COLS));
}

void updateWelcomeScroll(bool firstTime) {
  static const String welcome = "Welcome to the React! Game. Press Buttons to Start";
  if (firstTime) {
    beginScroll(welcome);
  } else {
    updateScroll();
  }
}

// Second LCD line, redrawn only when the level changes.
void showDifficultyLevel(int level) {
  String levelStr;
  switch (level) {
    case 1:
      levelStr = "1 (Easy)";
      break;
    case 2:
      levelStr = "2 (Medium)";
      break;
    case 3:
      levelStr = "3 (Hard)";
      break;
    case 4:
      levelStr = "4 (Expert)";
      break;
    default:
      levelStr = "Undefined";
      break;
  }

  String line = "Level: " + levelStr;
  while (line.length() < LCD_COLS) {
    line += ' ';
  }
  lcd.setCursor(0, 1);
  lcd.print(line);
}

void showGoMessage() {
  clearDisplay();
  lcd.setCursor(0, 0);
  lcd.print("Go!");
}

void showPlayerScores(int player1Score, int player2Score, bool force) {
  if (!force && scoreLineValid &&
      player1Score == lastScoreP1 && player2Score == lastScoreP2) {
    return;
  }

  char p1Text[8];
  char p2Text[8];
  snprintf(p1Text, sizeof(p1Text), "P1:%d", player1Score);
  snprintf(p2Text, sizeof(p2Text), "P2:%d", player2Score);

  // Clear the line, then left-align P1 and right-align P2.
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print(p1Text);

  int p2Column = LCD_COLS - (int)strlen(p2Text);
  if (p2Column < 8) {
    p2Column = 8;
  }
  lcd.setCursor(p2Column, 3);
  lcd.print(p2Text);

  lastScoreP1 = player1Score;
  lastScoreP2 = player2Score;
  scoreLineValid = true;
}

void showNextNumber(int number) {
  lcd.setCursor(0, 1);
  lcd.print("Press ");
  lcd.print(number);
  if (number == 1) {
    lcd.print(" time!   ");
  } else {
    lcd.print(" times!  ");
  }
}

void showRoundNumber(int round) {
  lcd.setCursor(0, 0);
  lcd.print("Round: ");
  lcd.print(round);
  lcd.print("        ");
}

void showEndGameMessage(bool firstTime, int winner) {
  if (!firstTime) {
    updateScroll();
    return;
  }

  if (winner == 1) {
    beginScroll("Game Over - The winner is P1");
  } else if (winner == 2) {
    beginScroll("Game Over - The winner is P2");
  } else {
    beginScroll("Game Over - Tie");
  }
}

void sleepDisplay() {
  lcd.clear();
  lcd.noDisplay();
  lcd.noBacklight();
}

void wakeDisplay() {
  lcd.display();
  lcd.backlight();
  clearDisplay();
}
