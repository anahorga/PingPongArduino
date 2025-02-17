#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <avr/interrupt.h>  // Necesită pentru întreruperi

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

bool willTheBallGoTowardsPlayerTwo = true;
bool isInputAllowed = true;
bool buttonProcessed = false;

const int playerOne = 12;
const int goalPlayerOne = 13;
const int buttonPlayerOne = 2;

const int playerTwo = 5;
const int goalPlayerTwo = 4;
const int buttonPlayerTwo = 3;
const int PIEZO = 1;

int scoreOfPlayerOne = 0;
int scoreOfPlayerTwo = 0;

const unsigned long initialMillisecondsPerLED = 500;
unsigned long millisecondsPerLED = initialMillisecondsPerLED;
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;

int currentPosition = playerOne;
int previousPosition = playerOne + 1;
int deltaPosition = 0;

volatile bool buttonOnePressed = false;
volatile bool buttonTwoPressed = false;

bool isShowingScores = false;
unsigned long scoreDisplayStartTime = 0;
const unsigned long scoreDisplayDuration = 3000;  // 3 secunde

void setup() {
    lcd.begin(16, 2);
    pinMode(1, OUTPUT);
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    pinMode(7, OUTPUT);
    pinMode(8, OUTPUT);
    pinMode(9, OUTPUT);
    pinMode(10, OUTPUT);
    pinMode(11, OUTPUT);
    pinMode(12, OUTPUT);
    pinMode(13, OUTPUT);

    pinMode(buttonPlayerOne, INPUT_PULLUP);
    pinMode(buttonPlayerTwo, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(buttonPlayerOne), isrButtonPlayerOne, FALLING);
    attachInterrupt(digitalPinToInterrupt(buttonPlayerTwo), isrButtonPlayerTwo, FALLING);

    // Configurarea temporizatorului pentru o întrerupere la fiecare 3 secunde
    cli();  // Dezactivăm întreruperile globale
    TCCR1A = 0;  // Configurăm registrul TCCR1A
    TCCR1B = 0;  // Configurăm registrul TCCR1B
    TCNT1 = 0;   // Resetăm contorul
    // Setăm prescaler-ul la 1024 și pornim temporizatorul
    TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);
    // Configurăm OCR1A pentru un interval de 3 secunde
    OCR1A = 46875;  // (16 MHz / (1024 * 1 Hz)) - 1 (pentru 1 Hz, 1 ciclu/secundă)
    TIMSK1 |= (1 << OCIE1A);  // Activăm întreruperea pentru comparație la OCR1A
    sei();  // Activăm întreruperile globale
}

ISR(TIMER1_COMPA_vect) {
    // Funcția de întrerupere care este apelată la fiecare 3 secunde
    if (millisecondsPerLED > 100) {  // Limităm viteza minimă pentru a nu deveni prea rapid
        millisecondsPerLED -= 50;  // Scădem durata pentru a crește viteza
    }
}

void loop() {
    if (isShowingScores) {
        if (millis() - scoreDisplayStartTime >= scoreDisplayDuration) {
            isShowingScores = false;
            lcd.clear();
            ResetValuesForNextRound();
        }
        return;
    }

    currentMillis = millis();

    if (buttonOnePressed && isInputAllowed && !buttonProcessed) {
        buttonOnePressed = false;
        buttonProcessed = true;
        if (willTheBallGoTowardsPlayerTwo) {
            ToggleBallDirection();
        }
    }

    if (buttonTwoPressed && isInputAllowed && !buttonProcessed) {
        buttonTwoPressed = false;
        buttonProcessed = true;
        if (!willTheBallGoTowardsPlayerTwo) {
            ToggleBallDirection();
        }
    }

    if (currentMillis - previousMillis >= millisecondsPerLED) {
        CheckGoalConditions();
        DetermineNextPosition();
        MoveBallToNextPosition();
        buttonProcessed = false;
        previousMillis = currentMillis;
    }
}

void isrButtonPlayerOne() {
    buttonOnePressed = true;
}

void isrButtonPlayerTwo() {
    buttonTwoPressed = true;
}

void ToggleBallDirection() {
    willTheBallGoTowardsPlayerTwo = !willTheBallGoTowardsPlayerTwo;
    isInputAllowed = false;
}

void MoveBallToNextPosition() {
    previousPosition = currentPosition;
    digitalWrite(previousPosition, LOW);
    currentPosition = currentPosition + deltaPosition;
    digitalWrite(currentPosition, HIGH);
    isInputAllowed = true;
}

void DetermineNextPosition() {
    deltaPosition = willTheBallGoTowardsPlayerTwo ? -1 : 1;
}

void ResetForNextRound() {
    isInputAllowed = true;
    buttonProcessed = false;
    currentPosition = playerOne;
    willTheBallGoTowardsPlayerTwo = true;
    previousMillis = millis();
    MoveBallToNextPosition();
}

void CheckGoalConditions() {
    if (currentPosition == goalPlayerTwo && willTheBallGoTowardsPlayerTwo) {
        ScoreForPlayer(1);
        ResetForNextRound();
    } else if (currentPosition == goalPlayerOne && !willTheBallGoTowardsPlayerTwo) {
        ScoreForPlayer(2);
        ResetForNextRound();
    }
}

void ScoreForPlayer(int playerWhoScored) {
    isInputAllowed = false;
    FlashAllLEDs(1, 0);
    if (playerWhoScored == 1) {
        scoreOfPlayerOne++;
        ShowScores(1);
    } else if (playerWhoScored == 2) {
        scoreOfPlayerTwo++;
        ShowScores(2);
    }
    CheckEndGame();
}

void CheckEndGame() {
    if (scoreOfPlayerOne == 3) {
        EndGameCeremonyFor(1);
        lcd.setCursor(0, 0);
        lcd.print("player one wins ");
        lcd.setCursor(0, 1);
        lcd.print("congratulations ");
        delay(2500);
        soundCorrectGuess();
        lcd.setCursor(0, 0);
        lcd.print("player 2,       ");
        lcd.setCursor(0, 1);
        //lcd.print("play again      ");
        delay(2500);
        soundBuzzer();
        lcd.setCursor(0, 0);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print("                ");
        delay(2500);
    }
    if (scoreOfPlayerTwo == 3) {
        EndGameCeremonyFor(2);
        lcd.setCursor(0, 0);
        lcd.print("player two wins ");
        lcd.setCursor(0, 1);
        lcd.print("congratulations ");
        delay(2500);
        soundCorrectGuess();
        lcd.setCursor(0, 0);
        //lcd.print("player 1,       ");
        lcd.setCursor(0, 1);
        lcd.print("play again      ");
        delay(2500);
        soundBuzzer();
        lcd.setCursor(0, 0);
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print("                ");
        delay(2500);
    }
}

void ShowScores(int playerCurrentlyScored) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Jucator1  ");
    lcd.print(scoreOfPlayerOne);
    lcd.setCursor(0, 1);
    lcd.print("Jucator2  ");
    lcd.print(scoreOfPlayerTwo);
    isShowingScores = true;
    scoreDisplayStartTime = millis();
}

void ResetValuesForNextRound() {
    //FlashAllLEDs(1, 0);
    millisecondsPerLED = initialMillisecondsPerLED;
}

void FlashAllLEDs(int blinkCount, int player) {
    for (int i = 0; i < blinkCount; i++) {
        TurnOnAllLEDsForPlayer(player);
        delay(35);
        TurnOffAllLEDsForPlayer(player);
        delay(35);
    }
}

void EndGameCeremonyFor(int winner) {
    FlashAllLEDs(50, winner);
    TurnOffAllLEDsForPlayer(0);
    scoreOfPlayerOne = 0;
    scoreOfPlayerTwo = 0;
}

void TurnOnAllLEDsForPlayer(int player) {
    if (player != 1) {
        digitalWrite(9, HIGH);
        digitalWrite(10, HIGH);
        digitalWrite(11, HIGH);
        digitalWrite(12, HIGH);
        digitalWrite(13, HIGH);
    }
    if (player != 2) {
        digitalWrite(4, HIGH);
        digitalWrite(5, HIGH);
        digitalWrite(6, HIGH);
        digitalWrite(7, HIGH);
        digitalWrite(8, HIGH);
    }
}

void TurnOffAllLEDsForPlayer(int player) {
    if (player != 1) {
        digitalWrite(4, LOW);
        digitalWrite(5, LOW);
        digitalWrite(6, LOW);
        digitalWrite(7, LOW);
        digitalWrite(8, LOW);
    }
    if (player != 2) {
        digitalWrite(9, LOW);
        digitalWrite(10, LOW);
        digitalWrite(11, LOW);
        digitalWrite(12, LOW);
        digitalWrite(13, LOW);
    }
}

void soundBuzzer() {
    tone(PIEZO, 100, 2000);
    delay(2300);
}

void soundCorrectGuess() {
    tone(PIEZO, 700, 100);
    delay(100);
    tone(PIEZO, 800, 100);
    delay(100);
    tone(PIEZO, 900, 100);
    delay(100);
    tone(PIEZO, 1000, 100);
    delay(100);
    tone(PIEZO, 1100, 100);
    delay(100);
    tone(PIEZO, 1200, 100);
    delay(100);
}
