#ifndef __INPUT__
#define __INPUT__

void initInput();
void resetInput();
bool isButtonPressed(int buttonIndex);
bool hasBeenPressed(int index);
bool areBothButtonsPressed();
void updateDifficultyLevel();
int getDifficultyLevel();
void inputPrepareForDeepSleep();
void inputResumeFromDeepSleep();
int getPlayerClicks(int playerIndex);
void startStageTimer(unsigned long durationMs);
void stopStageTimer();
bool isStageTimerExpired();
void resetStageTimerFlag();
#endif
