#ifndef __KERNEL__
#define __KERNEL__

#include <Arduino.h>

// #define __DEBUG__

/* general state management procedure */

void initKernel();
void updateStateTime();
void changeState(int new_state);

bool hasSystemTickPassed();
int getCurrentState();
long getCurrentTimeInState();
long getEnteredStateTime();
bool isJustEnteredInState();

/* for debugging */
void logMsg(const String& msg);
 
#endif
