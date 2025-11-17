/*********************************
* Simple led blinking 
* With Serial link at 115200 bauds
* Blink Onboard Led on D4
* Push Button on D7 (PullUp) => scrutation
* Board : ESP-D1  
* Author : O. Patrouix ESTIA
* Date : 29/09/2021
*********************************/

#include "ESP-D1_GPIO.h"

/* ------------------------- CONFIGURATION PINS ------------------------- */
const int LED_PIN = D5;
const int BUTTON_PIN = D7;   // Pull-up interne → appuyé = LOW

/* --------------------------- ETATS COURANTS --------------------------- */
int buttonState = LOW;      // état du bouton D7
int ledState = LOW;          // état LED D5

/* ----------------------------- TIMERS --------------------------------- */
long previousMillisBtn = 0;       // Comparatif utilisé par la fonction Millis
const long buttonInterval = 50;   // lecture toutes les 50 ms (anti rebond)

/* --------------------------- SERIAL BUFFER ---------------------------- */
String inputString = "";
bool stringComplete = false;


/* =======================================================================
   SETUP
   ======================================================================= */
void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState);

  pinMode(BUTTON_PIN, INPUT_PULLUP);  // IMPORTANT

  Serial.begin(115200);
  delay(300);
  Serial.println("\nBOOT ESP-D1 ready to work");
}


/* =======================================================================
   doEvery : exécuter une action toutes les X millisecondes
   ======================================================================= */
void doEvery(long intervalMillis, long *previousMillis, void (*action)()) {
  unsigned long currentMillis = millis();

  if (currentMillis - *previousMillis >= intervalMillis) {
    *previousMillis = currentMillis;
    action();
  }
}


/* =======================================================================
   LOOP
   ======================================================================= */
void loop() {

  // Lecture bouton périodique
  doEvery(buttonInterval, &previousMillisBtn, readButton);

  // Gestion entrée série
  serialEvent();

  if (stringComplete) {
    Serial.print(inputString);
    inputString = "";
    stringComplete = false;
  }
}


/* =======================================================================
   Fonction de lecture du bouton
   ======================================================================= */
void readButton() {

  int state = digitalRead(BUTTON_PIN);

  if (state == LOW) {         // bouton appuyé
    buttonState = LOW;
    ledState = LOW;
  } else {                    // bouton relâché
    buttonState = HIGH;
    ledState = HIGH;
  }

  digitalWrite(LED_PIN, ledState);

  Serial.print("Bouton pressé : ");
  Serial.println(buttonState);
}


/* =======================================================================
   Réception série non bloquante
   ======================================================================= */
void serialEvent() {

  while (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    inputString += inChar;

    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

