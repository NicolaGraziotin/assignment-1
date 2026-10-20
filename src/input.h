#ifndef __INPUT__
#define __INPUT__

void initInput();
void resetInput();
int takeButtonPresses(int buttonIndex);
bool isButtonPressed(int buttonIndex);
bool areBothButtonsPressed();
unsigned long getLastPressTime(int buttonIndex);
void updateDifficultyLevel();
int getDifficultyLevel();
void inputPrepareForDeepSleep();
void inputResumeFromDeepSleep();

#endif
