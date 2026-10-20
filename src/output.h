#ifndef __OUTPUT__
#define __OUTPUT__

void initOutput();
void clearDisplay();
void turnOffAllLeds();
void setGreenLed(int player, bool state);
void setRedLed(int player, bool state);
void setBothRedLeds(bool state);
void sleepDisplay();
void wakeDisplay();
void updateWelcomeScroll(bool firstTime);
void showDifficultyLevel(int level);
void showGoMessage();
void pulseRedLedsInCounterphase(unsigned long timeMs);
void showPlayerScores(int player1Score, int player2Score, bool force);
void showNextNumber(int number);
void showRoundNumber(int round);
void showEndGameMessage(bool firstTime, int winner);

#endif
