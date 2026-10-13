#ifndef __CONFIG__
#define __CONFIG__

// #define __DEBUG__

#define NUM_BUTTONS 2
#define NUM_LEDS 4

// player 1: L1 green, L2 red
#define GREEN_LED_PLAYER_1 13
#define RED_LED_PLAYER_1 10 // PWM
#define BUTTON_PLAYER_1 2

// player 2: L3 red, L4 green
#define RED_LED_PLAYER_2 11 // PWM
#define GREEN_LED_PLAYER_2 12
#define BUTTON_PLAYER_2 3

// difficulty level potentiometer
#define POTENTIOMETER A0

#endif