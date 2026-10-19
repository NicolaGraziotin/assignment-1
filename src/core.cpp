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

    Serial.print("Random wait time before GO: ");
    Serial.println(goTimeMs);

    Serial.print("Flag subito dopo avvio: ");
    Serial.println(isStageTimerExpired());
  }

  if (isStageTimerExpired()) {
    Serial.println("TIMER SCADUTO");
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

  // 2. MOMENTO DEL VIA! Il timer di attesa casuale è scaduto
  stopStageTimer();

  targetNumber = random(1, 4);
  showTargetNumber(targetNumber);
  showPlayersScore(scoreP1, scoreP2);
  resetInput();

  changeState(STAGE2_STATE);
}

#define RESULT_DISPLAY_TIME 10000 // 10 secondi per visualizzazione risultato

void stage2() {
  bool firstTime = isJustEnteredInState();
  static bool roundEnded = false;
  static unsigned long roundEndTime = 0;
  static int currentWinner = 0;

  if (firstTime) {
    Serial.println("Stage 2 started.");
    resetInput();
    roundEnded = false;
    roundEndTime = 0;

    // Avvia il timer limite del round T1
    startStageTimer(T1);
  }

  // Gestione della pausa asincrona di fine round
  if (roundEnded) {
    if (currentWinner != 0) showWinner(currentWinner, false);
    if (millis() - roundEndTime >= RESULT_DISPLAY_TIME) {
      roundEnded = false;
      roundEndTime = 0;
      turnOffAllLeds();
      clearDisplay();
      changeState(STAGE1_STATE);
    }
    return;
  }

  // Controllo Timeout del Round
  if (isStageTimerExpired()) {
    stopStageTimer();
    Serial.println("Time's up!");
    showTie(true);
    currentWinner = 0; // Nessun vincitore
    roundEnded = true;
    roundEndTime = millis();
    return;
  }

  // Controllo click dei giocatori...
  int clicksP1 = getPlayerClicks(0);
  int clicksP2 = getPlayerClicks(1);

  if (clicksP1 == targetNumber) {
    stopStageTimer();
    showWinner(1, true);
    currentWinner = 1;
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

    roundEnded = true;
    roundEndTime = millis();
    return;
  }

  // 6. Controllo Condizioni di Vittoria / Sconfitta Player 2
  if (clicksP2 == targetNumber) {
    stopStageTimer();

    Serial.println("Player 2 vince il round!");
    showWinner(2, true);
    currentWinner = 2;
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

  // game CHANGE interrupts off, pin-change wake-up on B1/B2
  inputPrepareForDeepSleep();

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu(); // ---- sleeps here, resumes here on B1/B2 ----

  sleep_disable();
  inputResumeFromDeepSleep();
  wakeDisplay();
  Serial.println("Woke up, back to intro.");
  changeState(INTRO_STATE);
}

void finalize(){}