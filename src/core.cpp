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
    changeState(STAGE1_STATE);
    return;
  }

  // 10 s without both buttons -> deep sleep
  if (getCurrentTimeInState() > INTRO_TIMEOUT){
    changeState(DEEP_SLEEP_STATE);
  }
}

void stage1(){
  bool firstTime = isJustEnteredInState();
  
  static int goTime = 0;
  static bool waitingForStart;
  targetNumber = random(1, 4);

  // 1. Setup iniziale dello Stage (eseguito 1 sola volta all'ingresso nello stato)
  if (firstTime){
    Serial.println("Stage 1 started.");
    showGoMessage();
    showPlayersScore(scoreP1, scoreP2);
    Serial.println("Go!");
    
    randomSeed(micros());
    goTime = random(0, MAX_WAIT); // Attesa casuale 0..MAX_WAIT
    waitingForStart = true;
    
    Serial.print("Random wait (ms): ");
    Serial.println(goTime);
  }

  // 2. Attesa dei 3 secondi di "Go!" + tempo casuale goTime
  if (waitingForStart) {
    if (getCurrentTimeInState() < goTime){
      return; // Continua ad attendere senza fare nulla
    }
    
    // Il tempo è scaduto: avvia IL ROUND vero e proprio (1 sola volta!)
    waitingForStart = false;
    
    showTargetNumber(targetNumber); // Mostra il numero generato sul display
    showPlayersScore(scoreP1, scoreP2); // Aggiorna il punteggio sul display
    resetInput(); // Azzera i contatori di click prima che il giocatore prema
    changeState(STAGE2_STATE); // Passa allo stato successivo per attendere la pressione dei pulsanti
  }
}

#define RESULT_DISPLAY_TIME 2000 // 2 secondi di visualizzazione per il risultato del round

void stage2(){
  bool firstTime = isJustEnteredInState();

  static bool roundEnded = false;
  static unsigned long roundEndTime = 0;
  static unsigned long roundStartTime = 0;

  // 1. Setup iniziale all'ingresso di Stage 2
  if (firstTime){
    Serial.println("Stage 2 started.");
    resetInput(); // Azzera i contatori di click accumulati
    roundEnded = false;
    roundEndTime = 0;
    roundStartTime = millis();
  }

  // 2. Se il round è già finito, attendi 2 secondi prima di ripartire/cambiare stato
  if (roundEnded) {
    if (millis() - roundEndTime >= RESULT_DISPLAY_TIME) {
      roundEnded = false;
      roundEndTime = 0;
      resetInput(); // Azzera i contatori di click accumulati
      targetNumber = random(1, 4); // Genera un nuovo numero target per il round successivo
      clearDisplay();
      delay(random(0, MAX_WAIT)); // Aspetta 1 secondo
      // Avvia il timer per il round successivo
      showTargetNumber(targetNumber); // Mostra il nuovo numero target sul display
      showPlayersScore(scoreP1, scoreP2); // Aggiorna il punteggio sul display
      turnOffAllLeds(); // Spegne tutti i LED prima del nuovo round
      roundStartTime = millis();
      return; // Continua a elaborare i click dei giocatori
    } else {
      return; // Non elaborare altri click durante i 2 secondi finali
    }
    return; // Non elaborare altri click durante i 2 secondi finali
  }

  // 3. Leggi i click correnti dei giocatori
  int clicksP1 = getPlayerClicks(0);
  int clicksP2 = getPlayerClicks(1);

  // Stampa di debug quando il conteggio varia
  static int lastClicksP1 = -1;
  static int lastClicksP2 = -1;

  if (clicksP1 != lastClicksP1 || clicksP2 != lastClicksP2) {
    lastClicksP1 = clicksP1;
    lastClicksP2 = clicksP2;
    
    Serial.print("P1: ");
    Serial.print(clicksP1);
    Serial.print(" / ");
    Serial.print(targetNumber);
    
    Serial.print(" | P2: ");
    Serial.print(clicksP2);
    Serial.print(" / ");
    Serial.println(targetNumber);
  }

  // 4. Controllo Vittoria / Sconfitta per Player 1
  if (T1 - (millis() - roundStartTime) <= 0) {
    Serial.println("Time's up! No winner this round.");
    showWinner(0); // Mostra nessun vincitore sul display
    roundEnded = true;
    roundEndTime = millis();
    return;
  }

  if (clicksP1 == targetNumber) {
    Serial.println("Player 1 vince il round!");
    showWinner(1); // Mostra il vincitore sul display
    scoreP1++;
    setLedState(1, true); // Accende L1 (LED verde Player 1)
    
    roundEnded = true;
    roundEndTime = millis();
    return;
  } else if (clicksP1 > targetNumber) {
    Serial.println("Player 1 ha superato il target!");
    scoreP1--;
    analogWrite(RED_LED_PLAYER_1, 255); // Accende L2 (LED rosso Player 1)
    
    roundEnded = true;
    roundEndTime = millis();
    return;
  }

  // 5. Controllo Vittoria / Sconfitta per Player 2
  if (clicksP2 == targetNumber) {
    Serial.println("Player 2 vince il round!");
    showWinner(2); // Mostra il vincitore sul display
    scoreP2++;
    setLedState(2, true); // Accende L4 (LED verde Player 2)
    
    roundEnded = true;
    roundEndTime = millis();
    return;
  } else if (clicksP2 > targetNumber) {
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
