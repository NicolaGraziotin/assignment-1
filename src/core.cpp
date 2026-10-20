#include <Arduino.h>
#include <avr/sleep.h>
#include <TimerOne.h>
#include "core.h"
#include "kernel.h"
#include "config.h"
#include "input.h"
#include "output.h"

#define INTRO_TIMEOUT 10000 // ms waiting for B1+B2 before deep sleep

static int scoreP1 = 0;
static int scoreP2 = 0;
static int lastShownLevel = -1;
static int targetNumber = 0;

static bool isFirstRoundAfterIntro = true;


void initCore(){
  Serial.begin(9600);
}

void resetScores(){
  scoreP1 = 0;
  scoreP2 = 0;
}

int getScoreP1(){ return scoreP1; }
int getScoreP2(){ return scoreP2; }

void intro(){
  bool firstTime = isJustEnteredInState();
  if (firstTime){
    Serial.println("Welcome to the React! Game. Press Buttons to Start");
    resetInput();
    turnOffAllLeds(); // greens off, reds driven by pulse below
    updateDifficultyLevel();
    lastShownLevel = getDifficultyLevel();
  }

  // single-line rotating message
  updateWelcomeScroll(firstTime);

  // difficulty selected with Pot
  if (firstTime){
    showDifficultyLevel(lastShownLevel);
  }
  updateDifficultyLevel();
  int level = getDifficultyLevel();
  if (level != lastShownLevel){
    lastShownLevel = level;
    showDifficultyLevel(level);
  }

  // L2 and L3 fading in/out in counterphase, greens stay off
  pulseRedLedsInCounterphase(getCurrentTimeInState());

  // start when BOTH buttons have been pressed
  if (areBothButtonsPressed()){
    turnOffAllLeds();
    resetScores();
    resetInput();
    isFirstRoundAfterIntro = true; // force first-time behavior in stage1
    changeState(STAGE1_STATE);
    return;
  }

  // 10 s without both buttons -> deep sleep
  if (getCurrentTimeInState() > INTRO_TIMEOUT){
    changeState(DEEP_SLEEP_STATE);
  }
}

void stage1() {
  bool firstTime = isJustEnteredInState();

  if (firstTime) {
    Serial.println("Stage 1 started. Waiting for GO...");

    // 1. Azzeramento input e pulizia display SEMPRE ad ogni ingresso
    resetInput();
    clearDisplay();
    showPlayersScore(scoreP1, scoreP2);

    // 2. Mostra "Go!" se arriviamo da intro, altrimenti il display resta pulito durante l'attesa
    if (isFirstRoundAfterIntro) {
      Serial.println("First round after intro, showing GO message.");
      showGoMessage();
      isFirstRoundAfterIntro = false;
    }

    // 3. Generazione e avvio del timer
    randomSeed(micros());
    unsigned long goTimeMs = random(2000, MAX_WAIT);
    startStageTimer(goTimeMs);
  }

  if (isStageTimerExpired()) {
    stopStageTimer();

    targetNumber = random(1, 4);
    showTargetNumber(targetNumber);
    showPlayersScore(scoreP1, scoreP2);
    resetInput();

    changeState(STAGE2_STATE);
    return;
  }

  // 1. Fase di attesa casuale e controllo Falsa Partenza
  if (!isStageTimerExpired()) {
    int clicksP1 = getPlayerClicks(0);
    int clicksP2 = getPlayerClicks(1);

    if (clicksP1 > 0 || clicksP2 > 0) {
      stopStageTimer();
      if (clicksP1 > 0) {
        Serial.println("Falsa partenza! Player 1");
        scoreP1--;
        analogWrite(RED_LED_PLAYER_1, 255);
      } else {
        Serial.println("Falsa partenza! Player 2");
        scoreP2--;
        analogWrite(RED_LED_PLAYER_2, 255);
      }
      resetInput();
      isFirstRoundAfterIntro = false;
      changeState(STAGE1_STATE); // Riavvia lo stage per riprovare
      return;
    }

    return; // Continua ad attendere lo scadere del timer
  }

  stopStageTimer();

  targetNumber = random(1, 4);
  showTargetNumber(targetNumber);
  showPlayersScore(scoreP1, scoreP2);
  resetInput();

  changeState(STAGE2_STATE);
}

#define ROUND_DISPLAY_TIME 2000       // 2 secondi per il vincitore del round o penalità
#define GAME_OVER_DISPLAY_TIME 10000  // 10 secondi per il Game Over finale

void stage2() {
  bool firstTime = isJustEnteredInState();
  static bool roundEnded = false;
  static unsigned long roundEndTime = 0;
  static int currentRoundWinner = 0; // >0 se un giocatore ha vinto il round, 0 se penalità, -1 se Game Over per timeout
  static int gameOverWinner =     0; // Vincitore finale della partita per timeout (1, 2 o 0 per tie)

  if (firstTime) {
    Serial.println("Stage 2 started.");
    resetInput();
    roundEnded = false;
    roundEndTime = 0;

    // Avvia il timer limite del round T1
    startStageTimer(T1);
  }

  // Gestione della pausa asincrona di fine round o fine partita
  if (roundEnded) {
    unsigned long currentDisplayTime = ROUND_DISPLAY_TIME;

    if (currentRoundWinner > 0) {
      showRoundWinner(currentRoundWinner, false);
      currentDisplayTime = ROUND_DISPLAY_TIME;
    } else if (currentRoundWinner == 0) {
      // Penalità per click in eccesso
      currentDisplayTime = ROUND_DISPLAY_TIME;
    } else if (currentRoundWinner == -1) {
      // Game Over finale (10 secondi)
      currentDisplayTime = GAME_OVER_DISPLAY_TIME;
      if (gameOverWinner == 0) {
        showTieGameMessage(false);
      } else {
        showWinner(gameOverWinner, false);
      }
    }

    if (millis() - roundEndTime >= currentDisplayTime) {
      roundEnded = false;
      roundEndTime = 0;
      turnOffAllLeds();
      clearDisplay();
      
      // Se è Game Over per timeout, torniamo a INTRO_STATE, altrimenti continuiamo i round
      if (currentRoundWinner == -1) {
        changeState(INTRO_STATE);
      } else {
        changeState(STAGE1_STATE);
      }
    }
    return;
  }

  // Controllo Timeout del Round (Game Over)
  if (isStageTimerExpired()) {
    stopStageTimer();
    Serial.println("Time's up! Game Over.");
    gameOverWinner = (scoreP1 > scoreP2) ? 1 : (scoreP2 > scoreP1) ? 2 : 0;
    
    if (gameOverWinner == 0) {
      showTieGameMessage(true);
    } else {
      showWinner(gameOverWinner, true);
    }

    currentRoundWinner = -1; // Segna che è Game Over per timeout
    roundEnded = true;
    roundEndTime = millis();
    return;
  }

  // Controllo click dei giocatori...
  int clicksP1 = getPlayerClicks(0);
  int clicksP2 = getPlayerClicks(1);

  if (clicksP1 == targetNumber) {
    stopStageTimer();
    showRoundWinner(1, true);
    currentRoundWinner = 1;
    scoreP1++;
    setLedState(1, true);
    roundEnded = true;
    roundEndTime = millis();
    return;
  } else if (clicksP1 > targetNumber) {
    stopStageTimer();
    Serial.println("Player 1 ha superato il target!");
    scoreP1--;
    analogWrite(RED_LED_PLAYER_1, 255); // Accende L2 (LED rosso Player 1)

    currentRoundWinner = 0; // Penalità
    roundEnded = true;
    roundEndTime = millis();
    return;
  }

  if (clicksP2 == targetNumber) {
    stopStageTimer();
    showRoundWinner(2, true);
    currentRoundWinner = 2;
    scoreP2++;
    setLedState(2, true); // Accende L4 (LED verde Player 2)
    roundEnded = true;
    roundEndTime = millis();
    return;
  } else if (clicksP2 > targetNumber) {
    stopStageTimer();
    Serial.println("Player 2 ha superato il target!");
    scoreP2--;
    analogWrite(RED_LED_PLAYER_2, 255); // Accende L3 (LED rosso Player 2)

    currentRoundWinner = 0; // Penalità
    roundEnded = true;
    roundEndTime = millis();
    return;
  }
}

void deepSleep(){
  bool firstTime = isJustEnteredInState();
  if (!firstTime){
    return;
  }
  Serial.println("Going to sleep. Press B1 or B2 to wake up.");
  Serial.flush();

  turnOffAllLeds();
  sleepDisplay();
  resetInput();

  inputPrepareForDeepSleep();

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();

  sleep_disable();
  inputResumeFromDeepSleep();
  wakeDisplay();
  Serial.println("Woke up, back to intro.");
  changeState(INTRO_STATE);
}

void finalize(){}