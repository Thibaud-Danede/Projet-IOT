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

/* ---------------- PINS ---------------- */
const int LED_PIN = D5;
const int BUTTON_PIN = D7;

/* ---------------- ETATS ---------------- */
int buttonState = LOW;
int ledState = LOW;

/* ---------------- TIMERS ---------------- */
long previousMillisBtn = 0;
long buttonInterval = 500;

/* ---------------- SERIAL ---------------- */
String inputString = "";
bool stringComplete = false;

/* ---------------- MENU ---------------- */
enum MenuState {
  MENU_MAIN,
  MENU_BUTTON
};

MenuState menu = MENU_MAIN;


/* =======================================================================
   SETUP
   ======================================================================= */
void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  delay(300);
  Serial.println("\nBOOT ESP-D1 ready to work");

  showMainMenu();
}

/* =======================================================================
   MENU AFFICHAGE
   ======================================================================= */
void showMainMenu() {
  Serial.println("\n======== MENU PRINCIPAL ========");
  Serial.println("Tapez dans l'invite de commande le paramètre que vous voulez régler");
  Serial.println("1 : Gérer l'acquisition du bouton");
  Serial.println("2 : (option future)");
}

void showButtonMenu() {
  Serial.println("\n=== MENU BOUTON ===");
  Serial.println("Affichage des valeurs du bouton...");
  Serial.println("Commandes disponibles :");
  Serial.println("  BUTTONSET <ms>   : changer intervalle lecture");
  Serial.println("  RETURN           : retour au menu principal\n");
}


/* =======================================================================
   LOOP
   ======================================================================= */
void loop() {

  // Lecture bouton seulement dans le menu bouton
  if (menu == MENU_BUTTON) {
    doEvery(buttonInterval, &previousMillisBtn, readButton);
  }

  // Lecture série
  serialEvent();

  if (stringComplete) {
    parseCommand(inputString);
    inputString = "";
    stringComplete = false;
  }
}


/* =======================================================================
   Fonction lecture bouton
   ======================================================================= */
void readButton() {
  int state = digitalRead(BUTTON_PIN);

  if (state == LOW) {
    buttonState = LOW;
    ledState = LOW;
  } else {
    buttonState = HIGH;
    ledState = HIGH;
  }

  digitalWrite(LED_PIN, ledState);

  Serial.print("Bouton pressé : ");
  Serial.println(buttonState);
}


/* =======================================================================
   Analyse des commandes utilisateur
   ======================================================================= */
void parseCommand(String cmd) {

  cmd.trim();

  /* === COMMANDE MENU PRINCIPAL === */
  if (menu == MENU_MAIN) {

    if (cmd == "1") {
      menu = MENU_BUTTON;
      showButtonMenu();
      return;
    }

    if (cmd == "2") {
      Serial.println("Option 2 non encore implémentée.");
      showMainMenu();
      return;
    }

    Serial.println("Commande inconnue.");
    showMainMenu();
    return;
  }


  /* === COMMANDE MENU BOUTON === */
  if (menu == MENU_BUTTON) {

    // Retour au menu principal
    if (cmd == "RETURN") {
      menu = MENU_MAIN;
      showMainMenu();
      return;
    }

    // BUTTONSET <ms>
    if (cmd.startsWith("BUTTONSET")) {

      String valueStr = cmd.substring(9);
      valueStr.trim();
      long newInterval = valueStr.toInt();

      if (newInterval > 0) {
        buttonInterval = newInterval;
        Serial.print("Nouvel intervalle = ");
        Serial.print(buttonInterval);
        Serial.println(" ms");
      } else {
        Serial.println("Erreur: donnée non valide.");
      }
      return;
    }

    Serial.println("Commande inconnue. Tapez RETURN pour revenir au menu principal.");
    return;
  }
}


/* =======================================================================
   Réception série
   ======================================================================= */
void serialEvent() {
  while (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n') stringComplete = true;
  }
}


/* =======================================================================
   doEvery
   ======================================================================= */
void doEvery(long intervalMillis, long *previousMillis, void (*action)()) {
  unsigned long currentMillis = millis();
  if (currentMillis - *previousMillis >= intervalMillis) {
    *previousMillis = currentMillis;
    action();
  }
}


