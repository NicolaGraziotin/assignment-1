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
 
#endif
