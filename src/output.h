#ifndef __OUTPUT__
#define __OUTPUT__

void initOutput();
void setLedState(int ledIndex, bool state);
void turnOnRedLed(int playerIndex);
void turnOnGreenLed(int playerIndex);
void turnOffAllLeds();
void sleepDisplay();
void wakeDisplay();
void printDifficultyLevel(int level);
void updateWelcomeScroll(bool firstTime);
void showDifficultyLevel(int level);
void showGoMessage();
void pulseRedLedsInCounterphase(long timeMs);
void showTargetNumber(int targetNumber);
void showPlayersScore(int scoreP1, int scoreP2);
void clearDisplay();
void showRoundWinner(int playerIndex, bool firstTime);
void showWinner(int playerIndex, bool firstTime);
void showTieGameMessage(bool firstTime);

#endif