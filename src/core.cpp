#include <Arduino.h>
#include <avr/sleep.h>
#include <TimerOne.h>
#include "core.h"
#include "kernel.h"
#include "config.h"
#include "input.h"
#include "output.h"

#define INTRO_TIMEOUT 10000 // ms di attesa per B1+B2 prima del deep sleep

static int scoreP1 = 0;
static int scoreP2 = 0;
static int lastShownLevel = -1;
static int targetNumber = 0;
static unsigned long currentT1 = T1; // Valore corrente dinamico di T1 che diminuisce round dopo round

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
    turnOffAllLeds(); 
    updateDifficultyLevel();
    lastShownLevel = getDifficultyLevel();
  }

  updateWelcomeScroll(firstTime);

  if (firstTime){
    showDifficultyLevel(lastShownLevel);
  }
  updateDifficultyLevel();
  int level = getDifficultyLevel();
  if (level != lastShownLevel){
    lastShownLevel = level;
    showDifficultyLevel(level);
  }

  pulseRedLedsInCounterphase(getCurrentTimeInState());

  if (areBothButtonsPressed()){
    turnOffAllLeds();
    resetScores();
    resetInput();
    
    // Ripristina T1 al valore iniziale configurato all'avvio della partita
    currentT1 = T1;
    
    isFirstRoundAfterIntro = true; 
    changeState(STAGE1_STATE);
    return;
  }

  if (getCurrentTimeInState() > INTRO_TIMEOUT){
    changeState(DEEP_SLEEP_STATE);
  }
}

#define ROUND_DISPLAY_TIME 2000       
#define GAME_OVER_DISPLAY_TIME 10000  

void stage1() {
  bool firstTime = isJustEnteredInState();
  static bool falseStartOccurred = false;
  static unsigned long falseStartTime = 0;

  if (firstTime) {
    Serial.println("Stage 1 started. Waiting for GO...");

    resetInput();
    clearDisplay();
    showPlayersScore(scoreP1, scoreP2);

    if (isFirstRoundAfterIntro) {
      Serial.println("First round after intro, showing GO message.");
      showGoMessage();
      isFirstRoundAfterIntro = false;
    }

    randomSeed(micros());
    unsigned long goTimeMs = random(2000, MAX_WAIT);
    startStageTimer(goTimeMs);

    falseStartOccurred = false;
    falseStartTime = 0;
  }

  // Se c'è stata una falsa partenza, attendiamo 2 secondi con il LED rosso acceso
  if (falseStartOccurred) {
    if (millis() - falseStartTime >= ROUND_DISPLAY_TIME) {
      falseStartOccurred = false;
      turnOffAllLeds();
      clearDisplay();
      changeState(STAGE1_STATE);
    }
    return;
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

  if (!isStageTimerExpired()) {
    int clicksP1 = getPlayerClicks(0);
    int clicksP2 = getPlayerClicks(1);

    if (clicksP1 > 0 || clicksP2 > 0) {
      stopStageTimer();
      if (clicksP1 > 0) {
        Serial.println("Falsa partenza! Player 1");
        scoreP1--;
        turnOnRedLed(1); 
      } else {
        Serial.println("Falsa partenza! Player 2");
        scoreP2--;
        turnOnRedLed(2); 
      }
      resetInput();
      showPlayersScore(scoreP1, scoreP2); 
      
      falseStartOccurred = true;
      falseStartTime = millis();
      isFirstRoundAfterIntro = false;
      return;
    }

    return; 
  }

  stopStageTimer();

  targetNumber = random(1, 4);
  showTargetNumber(targetNumber);
  showPlayersScore(scoreP1, scoreP2);
  resetInput();

  changeState(STAGE2_STATE);
}

void stage2() {
  bool firstTime = isJustEnteredInState();
  static bool roundEnded = false;
  static unsigned long roundEndTime = 0;
  static int currentRoundWinner = 0; 
  static int gameOverWinner = 0;     

  if (firstTime) {
    Serial.println("Stage 2 started.");
    resetInput();
    roundEnded = false;
    roundEndTime = 0;

    // Avvia il timer con il tempo corrente T1 (che diminuisce ad ogni round completato)
    startStageTimer(currentT1);
  }

  if (roundEnded) {
    unsigned long currentDisplayTime = ROUND_DISPLAY_TIME;

    if (currentRoundWinner > 0) {
      showRoundWinner(currentRoundWinner, false);
      currentDisplayTime = ROUND_DISPLAY_TIME;
    } else if (currentRoundWinner == 0) {
      currentDisplayTime = ROUND_DISPLAY_TIME;
    } else if (currentRoundWinner == -1) {
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
      
      if (currentRoundWinner == -1) {
        changeState(INTRO_STATE);
      } else {
        // ROUND COMPLETATO CON SUCCESSO: Riduciamo T1 di un fattore F dipendente dalla difficoltà L (1..4)
        int level = getDifficultyLevel();
        // Calcoliamo F in base al livello (es. livello 1 -> F=200ms, livello 4 -> F=800ms)
        unsigned long factorF = level * F; 
        
        if (currentT1 > factorF + 500) { // Mettiamo una soglia di sicurezza minima (es. 0,5 secondi)
          currentT1 -= factorF;
        } else {
          currentT1 = 500; // Soglia minima invalicabile per il tempo di reazione
        }
        
        Serial.print("Round completato. Nuovo T1 (ms): ");
        Serial.println(currentT1);

        changeState(STAGE1_STATE);
      }
    }
    return;
  }

  // Controllo Timeout del Round (Nessuno ha risposto in tempo -> Game Over)
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

  int clicksP1 = getPlayerClicks(0);
  int clicksP2 = getPlayerClicks(1);

  // Vittoria del round per P1 (raggiunto esattamente N click)
  if (clicksP1 == targetNumber) {
    stopStageTimer();
    showRoundWinner(1, true);
    currentRoundWinner = 1;
    scoreP1++;
    turnOnGreenLed(1); 
    roundEnded = true;
    roundEndTime = millis();
    return;
  } 
  // Errore P1 (click in eccesso oltre N)
  else if (clicksP1 > targetNumber) {
    stopStageTimer();
    Serial.println("Player 1 ha superato il target!");
    scoreP1--;
    turnOnRedLed(1); 
    showPlayersScore(scoreP1, scoreP2);

    currentRoundWinner = 0; // Penalità (il round finisce ma T1 NON si riduce per merito)
    roundEnded = true;
    roundEndTime = millis();
    return;
  }

  // Vittoria del round per P2 (raggiunto esattamente N click)
  if (clicksP2 == targetNumber) {
    stopStageTimer();
    showRoundWinner(2, true);
    currentRoundWinner = 2;
    scoreP2++;
    turnOnGreenLed(2); 
    roundEnded = true;
    roundEndTime = millis();
    return;
  } 
  // Errore P2 (click in eccesso oltre N)
  else if (clicksP2 > targetNumber) {
    stopStageTimer();
    Serial.println("Player 2 ha superato il target!");
    scoreP2--;
    turnOnRedLed(2); 
    showPlayersScore(scoreP1, scoreP2);

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