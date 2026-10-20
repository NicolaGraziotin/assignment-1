#ifndef __CORE__
#define __CORE__

#define INTRO_STATE   1
#define STAGE1_STATE  2
#define STAGE2_STATE  3
#define DEEP_SLEEP_STATE 4
#define FINAL_STATE   5
#define GO_STATE      6
#define ROUND_END_STATE 7
#define GAME_OVER_LED_STATE 8

/* core business logic  */

void initCore();
void intro();
void goState();
void stage1();
void stage2();
void roundEnd();
void gameOverLeds();
void deepSleep();
void finalize();

void resetScores();
int getScoreP1();
int getScoreP2();

#endif
