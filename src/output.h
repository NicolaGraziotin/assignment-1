#ifndef __OUTPUT__
#define __OUTPUT__

void initOutput();
void setLedState(int ledIndex, bool state);
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
void showWinner(int playerIndex);

#endif