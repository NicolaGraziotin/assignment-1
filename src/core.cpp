#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "core.h"
#include "kernel.h"
#include "config.h"
#include "input.h"
#include "output.h"

static int roundNumber = 1;
static int scoreP1 = 0;
static int scoreP2 = 0;
static unsigned long currentT1Ms = T1;
static int selectedLevel = 1;  // level chosen before the game starts
static int lastShownLevel = -1;
static int finalWinner = 0;

/* Per-round state. */
static int counterPlayer1 = 0;
static int counterPlayer2 = 0;
static int currentNumber = 1;
static int roundWinner = -1;         // player who gets the green led
static int roundPenaltyPlayer = -1;  // player who gets the red led
static bool bothPenalty = false;
static int pendingWinner = -1;  // reached N, waiting for confirmation
static unsigned long winConfirmStart = 0;
static unsigned long reactionDeadline = 0;

/* Random wait before N is shown, and early-press penalty. */
static unsigned long waitDeadline = 0;
static unsigned long penaltyStartMs = 0;
static unsigned long penaltyRemainingMs = 0;
static bool penaltyActive = false;
static bool penaltyPlayer1 = false;
static bool penaltyPlayer2 = false;

/* millis()-safe deadline test (also correct across the millis() wrap). */
static bool timeReached(unsigned long deadline) {
  return (long)(millis() - deadline) >= 0;
}

static int getReductionPercent() {
  switch (selectedLevel) {
    case 1:
      return F_LEVEL_1;
    case 2:
      return F_LEVEL_2;
    case 3:
      return F_LEVEL_3;
    case 4:
      return F_LEVEL_4;
    default:
      return F_LEVEL_1;
  }
}

static void reduceT1() {
  int percent = getReductionPercent();
  unsigned long reduced = (currentT1Ms * (100 - percent)) / 100;
  if (reduced < T1_MIN) {
    reduced = T1_MIN;
  }
  currentT1Ms = reduced;
}

static int determineWinner() {
  if (scoreP1 > scoreP2) {
    return 1;
  }
  if (scoreP2 > scoreP1) {
    return 2;
  }
  return 0;
}

static void addPoint(int player) {
  if (player == PLAYER_1_ID) {
    scoreP1++;
  } else if (player == PLAYER_2_ID) {
    scoreP2++;
  }
}

static void removePoint(int player) {
  if (player == PLAYER_1_ID) {
    scoreP1--;
  } else if (player == PLAYER_2_ID) {
    scoreP2--;
  }
}

static void clearRoundOutcome() {
  roundWinner = -1;
  roundPenaltyPlayer = -1;
  bothPenalty = false;
}

static void setRoundWinner(int player) {
  roundWinner = player;
  roundPenaltyPlayer = -1;
  bothPenalty = false;
}

static void setRoundPenalty(int player) {
  roundWinner = -1;
  roundPenaltyPlayer = player;
  bothPenalty = false;
}

static void setBothPenalties() {
  roundWinner = -1;
  roundPenaltyPlayer = -1;
  bothPenalty = true;
}

static void resetCounters() {
  counterPlayer1 = 0;
  counterPlayer2 = 0;
}

static void resetEarlyPenalty() {
  penaltyActive = false;
  penaltyPlayer1 = false;
  penaltyPlayer2 = false;
}

static void discardAllPresses() {
  takeButtonPresses(PLAYER_1_ID);
  takeButtonPresses(PLAYER_2_ID);
}

static void beginIntro() {
  resetInput();
  turnOffAllLeds();
  currentT1Ms = T1;
  roundNumber = 1;

  // A new game will start from zero, but the final scores remain
  // available until then for debugging.
  lastShownLevel = -1;
  updateDifficultyLevel();
  lastShownLevel = getDifficultyLevel();

  updateWelcomeScroll(true);
  showDifficultyLevel(lastShownLevel);
  DEBUG_PRINTLN("Welcome to the React! Game. Press Buttons to Start");
}

static void updateIntro() {
  updateWelcomeScroll(false);

  updateDifficultyLevel();
  int level = getDifficultyLevel();
  if (level != lastShownLevel) {
    lastShownLevel = level;
    showDifficultyLevel(level);
  }
}

static void beginGame() {
  resetScores();
  roundNumber = 1;
  currentT1Ms = T1;
  selectedLevel = getDifficultyLevel();
  clearRoundOutcome();
  resetInput();
  turnOffAllLeds();
  showGoMessage();
  DEBUG_PRINTLN("Game started.");
}

static void beginRandomWait() {
  turnOffAllLeds();
  clearDisplay();
  resetInput();
  resetCounters();
  clearRoundOutcome();
  resetEarlyPenalty();
  waitDeadline = millis() + random(0, MAX_WAIT + 1);
  DEBUG_PRINT("Round ");
  DEBUG_PRINTLN(roundNumber);
}

static void beginEarlyPenalty(int pressesP1, int pressesP2) {
  penaltyPlayer1 = (pressesP1 > 0);
  penaltyPlayer2 = (pressesP2 > 0);

  if (penaltyPlayer1) {
    removePoint(PLAYER_1_ID);
  }
  if (penaltyPlayer2) {
    removePoint(PLAYER_2_ID);
  }

  turnOffAllLeds();
  if (penaltyPlayer1) {
    setRedLed(PLAYER_1_ID, true);
  }
  if (penaltyPlayer2) {
    setRedLed(PLAYER_2_ID, true);
  }

  penaltyRemainingMs = timeReached(waitDeadline) ? 0 : (waitDeadline - millis());
  penaltyStartMs = millis();
  penaltyActive = true;

  clearDisplay();
  showPlayerScores(scoreP1, scoreP2, true);
  DEBUG_PRINTLN("Too early!");
}

static void endEarlyPenalty() {
  turnOffAllLeds();
  resetEarlyPenalty();
  clearDisplay();

  // Resume the random wait where it was interrupted.
  waitDeadline = millis() + penaltyRemainingMs;
}

static void beginReactionRound() {
  resetInput();
  resetCounters();
  pendingWinner = -1;
  currentNumber = random(1, MAX_NUMBER + 1);
  clearDisplay();
  showRoundNumber(roundNumber);
  showNextNumber(currentNumber);
  DEBUG_PRINT("Number: ");
  DEBUG_PRINTLN(currentNumber);
  DEBUG_PRINT("T1: ");
  DEBUG_PRINTLN(currentT1Ms);

  // Start the reaction time once N is actually visible on the LCD.
  reactionDeadline = millis() + currentT1Ms;
}

/*
 * A player has reached N. Wait a short confirmation window before awarding
 * the point: if the same player presses again, the round is a foul instead.
 * This makes the "more than N presses" penalty reliable even when each press
 * is read in a different super-loop iteration.
 */
static void updatePendingWinner() {
  if (pendingWinner == PLAYER_1_ID && counterPlayer1 > currentNumber) {
    removePoint(PLAYER_1_ID);
    setRoundPenalty(PLAYER_1_ID);
    pendingWinner = -1;
    changeState(ROUND_END_STATE);
    return;
  }

  if (pendingWinner == PLAYER_2_ID && counterPlayer2 > currentNumber) {
    removePoint(PLAYER_2_ID);
    setRoundPenalty(PLAYER_2_ID);
    pendingWinner = -1;
    changeState(ROUND_END_STATE);
    return;
  }

  if (millis() - winConfirmStart >= WIN_CONFIRM_MS) {
    addPoint(pendingWinner);
    setRoundWinner(pendingWinner);
    pendingWinner = -1;
    changeState(ROUND_END_STATE);
  }
}

/*
 * If both players completed N in the same loop iteration, compare the
 * timestamps of their last accepted press to decide who was actually first.
 * Returns true when a pending winner has been selected.
 */
static bool selectPendingWinner(bool &p1Exact, bool &p2Exact) {
  if (p1Exact && p2Exact) {
    if (getLastPressTime(PLAYER_1_ID) <= getLastPressTime(PLAYER_2_ID)) {
      p2Exact = false;
    } else {
      p1Exact = false;
    }
  }

  if (!p1Exact && !p2Exact) {
    return false;
  }

  pendingWinner = p1Exact ? PLAYER_1_ID : PLAYER_2_ID;
  winConfirmStart = millis();
  return true;
}

/* At least one player exceeded N: apply the penalty and end the round. */
static void handleOvershoot(bool p1Over, bool p2Over) {
  if (p1Over && p2Over) {
    removePoint(PLAYER_1_ID);
    removePoint(PLAYER_2_ID);
    setBothPenalties();
  } else if (p1Over) {
    removePoint(PLAYER_1_ID);
    setRoundPenalty(PLAYER_1_ID);
  } else if (p2Over) {
    removePoint(PLAYER_2_ID);
    setRoundPenalty(PLAYER_2_ID);
  }

  changeState(ROUND_END_STATE);
}

static void showRoundResult() {
  turnOffAllLeds();

  if (roundWinner == PLAYER_1_ID) {
    setGreenLed(PLAYER_1_ID, true);
  } else if (roundWinner == PLAYER_2_ID) {
    setGreenLed(PLAYER_2_ID, true);
  } else if (bothPenalty) {
    setBothRedLeds(true);
  } else if (roundPenaltyPlayer == PLAYER_1_ID) {
    setRedLed(PLAYER_1_ID, true);
  } else if (roundPenaltyPlayer == PLAYER_2_ID) {
    setRedLed(PLAYER_2_ID, true);
  }

  clearDisplay();
  showRoundNumber(roundNumber);
  showPlayerScores(scoreP1, scoreP2, true);
}

static void startNextRound() {
  turnOffAllLeds();
  reduceT1();
  roundNumber++;
  changeState(STAGE1_STATE);
}

static void beginGameOverLeds() {
  turnOffAllLeds();
  setBothRedLeds(true);
  clearDisplay();
  DEBUG_PRINTLN("Timeout: game over.");
}

static void beginFinalMessage() {
  finalWinner = determineWinner();

  turnOffAllLeds();
  resetInput();
  showEndGameMessage(true, finalWinner);

  DEBUG_PRINT("Final score - P1: ");
  DEBUG_PRINT(scoreP1);
  DEBUG_PRINT(" P2: ");
  DEBUG_PRINTLN(scoreP2);
}

static void disableTimerInterruptsForSleep() {
  // Avoid a pending timer tick from waking the MCU immediately.
  TIMSK0 &= ~(1 << TOIE0);
  TIMSK1 &= ~(1 << OCIE1A);
}

static void enableTimerInterruptsAfterSleep() {
  TIMSK0 |= (1 << TOIE0);
  TIMSK1 |= (1 << OCIE1A);
}

static void powerDownUntilButtonPress() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();  // ---- sleeps here, resumes here on B1/B2 ----
  sleep_disable();
}

void initCore() {
#ifdef __DEBUG__
  Serial.begin(9600);
#endif

  // Seed the Arduino PRNG from the noise of an unconnected analog pin.
  unsigned long seed = 0;
  for (int i = 0; i < 8; i++) {
    seed = seed * 33UL + analogRead(A1);
  }
  randomSeed(seed ^ micros());

  resetScores();
  currentT1Ms = T1;
}

void resetScores() {
  scoreP1 = 0;
  scoreP2 = 0;
}

void intro() {
  if (isJustEnteredInState()) {
    beginIntro();
  } else {
    updateIntro();
  }

  // L2 and L3 fade in/out in counterphase; green leds stay off.
  pulseRedLedsInCounterphase(getCurrentTimeInState());

  // Start when BOTH buttons have been pressed.
  if (areBothButtonsPressed()) {
    changeState(GO_STATE);
    return;
  }

  // No game start within INTRO_TIMEOUT -> deep sleep.
  if (getCurrentTimeInState() >= INTRO_TIMEOUT) {
    changeState(DEEP_SLEEP_STATE);
  }
}

void goState() {
  if (isJustEnteredInState()) {
    beginGame();
  }

  if (getCurrentTimeInState() >= GO_DURATION) {
    resetInput();
    changeState(STAGE1_STATE);
  }
}

/*
 * STAGE1: leds off, LCD cleared, wait a random time before showing N.
 * Presses in this phase are too early and cause a 2 s red-led penalty.
 */
void stage1() {
  if (isJustEnteredInState()) {
    beginRandomWait();
  }

  if (penaltyActive) {
    // Presses while the penalty is displayed are discarded.
    discardAllPresses();

    if (!timeReached(penaltyStartMs + ROUND_RESULT_DURATION)) {
      return;
    }

    endEarlyPenalty();
    return;
  }

  int pressesP1 = takeButtonPresses(PLAYER_1_ID);
  int pressesP2 = takeButtonPresses(PLAYER_2_ID);

  if (pressesP1 > 0 || pressesP2 > 0) {
    beginEarlyPenalty(pressesP1, pressesP2);
    return;
  }

  if (timeReached(waitDeadline)) {
    changeState(STAGE2_STATE);
  }
}

/*
 * STAGE2: N is shown; the first player who presses it exactly N times wins.
 * Pressing more than N times causes a penalty.
 */
void stage2() {
  if (isJustEnteredInState()) {
    beginReactionRound();
  }

  counterPlayer1 += takeButtonPresses(PLAYER_1_ID);
  counterPlayer2 += takeButtonPresses(PLAYER_2_ID);

  if (pendingWinner != -1) {
    updatePendingWinner();
    return;
  }

  bool p1Exact = (counterPlayer1 == currentNumber);
  bool p2Exact = (counterPlayer2 == currentNumber);
  bool p1Over = (counterPlayer1 > currentNumber);
  bool p2Over = (counterPlayer2 > currentNumber);

  if (p1Exact || p2Exact || p1Over || p2Over) {
    if (selectPendingWinner(p1Exact, p2Exact)) {
      return;
    }

    if (p1Over || p2Over) {
      handleOvershoot(p1Over, p2Over);
      return;
    }
  }

  if (timeReached(reactionDeadline)) {
    changeState(GAME_OVER_LED_STATE);
  }
}

/* Show the green/red result led and the scores for 2 seconds. */
void roundEnd() {
  if (isJustEnteredInState()) {
    showRoundResult();
  }

  if (getCurrentTimeInState() < ROUND_RESULT_DURATION) {
    return;
  }

  startNextRound();
}

/* Timeout: both red leds on for 2 seconds, then game over. */
void gameOverLeds() {
  if (isJustEnteredInState()) {
    beginGameOverLeds();
  }

  if (getCurrentTimeInState() >= GAME_OVER_LED_DURATION) {
    turnOffAllLeds();
    changeState(FINAL_STATE);
  }
}

void finalize() {
  if (isJustEnteredInState()) {
    beginFinalMessage();
  } else {
    showEndGameMessage(false, finalWinner);
  }

  if (getCurrentTimeInState() >= FINAL_TIMEOUT) {
    changeState(INTRO_STATE);
  }
}

void deepSleep() {
  if (!isJustEnteredInState()) {
    return;
  }

  turnOffAllLeds();
  sleepDisplay();
  resetInput();
  DEBUG_PRINTLN("Going to sleep. Press B1 or B2 to wake up.");
#ifdef __DEBUG__
  Serial.flush();
#endif

  // Game CHANGE interrupts off, pin-change wake-up on B1/B2.
  inputPrepareForDeepSleep();

  disableTimerInterruptsForSleep();
  powerDownUntilButtonPress();
  enableTimerInterruptsAfterSleep();

  inputResumeFromDeepSleep();
  wakeDisplay();
  DEBUG_PRINTLN("Woke up, back to intro.");
  changeState(INTRO_STATE);
}
