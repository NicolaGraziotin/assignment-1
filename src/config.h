#ifndef __CONFIG__
#define __CONFIG__

// #define __DEBUG__

#ifdef __DEBUG__
#define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#endif

#define NUM_BUTTONS 2
#define NUM_LEDS 4

// player 1: L1 green, L2 red
#define GREEN_LED_PLAYER_1 13
#define RED_LED_PLAYER_1 6 // PWM
#define BUTTON_PLAYER_1 2

// player 2: L4 green, L3 red
#define GREEN_LED_PLAYER_2 12
#define RED_LED_PLAYER_2 11 // PWM
#define BUTTON_PLAYER_2 3

// difficulty level potentiometer
#define POTENTIOMETER A0

/* Game parameters (all times in milliseconds) */
#define T1 5000        // initial max reaction time
#define T1_MIN 700     // minimum reaction time reachable after reductions
#define MAX_WAIT 5000  // max random delay before showing N
#define MAX_NUMBER 3   // N is in 1..MAX_NUMBER

/*
 * Difficulty level -> percentage by which T1 is reduced after every round.
 * Bigger percentage = steeper difficulty increase.
 */
#define F_LEVEL_1 4   // easiest
#define F_LEVEL_2 7
#define F_LEVEL_3 10
#define F_LEVEL_4 15  // hardest

#define INTRO_TIMEOUT 10000          // no start -> deep sleep
#define GO_DURATION 1000             // "Go!" message duration
#define WIN_CONFIRM_MS 250           // over-press check after reaching N
#define ROUND_RESULT_DURATION 2000   // green/red result led duration
#define GAME_OVER_LED_DURATION 2000  // both red leds after a timeout
#define FINAL_TIMEOUT 10000          // game-over message duration

#define PLAYER_1_ID 0
#define PLAYER_2_ID 1

#endif
