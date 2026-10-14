#ifndef __OUTPUT__
#define __OUTPUT__

void initOutput();
void setLedState(int ledIndex, bool state);
void turnOffAllLeds();
void sleepDisplay();
void wakeDisplay();
void updateWelcomeScroll(bool firstTime);
void showDifficultyLevel(int level);
void showGoMessage();
void pulseRedLedsInCounterphase(long timeMs);
void showPlayerScores(int player1Score, int player2Score);

#endif