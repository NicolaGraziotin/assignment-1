#ifndef __OUTPUT__
#define __OUTPUT__

void initOutput();
void setLedState(int ledIndex, bool state);
void printDifficultyLevel(int level);
void updateWelcomeScroll(bool firstTime);
void pulseRedLedsInCounterphase(int step);

#endif