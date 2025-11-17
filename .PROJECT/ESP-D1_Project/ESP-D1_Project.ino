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

#include "DHT.h"

#define DHTTYPE DHT11

/* ---------------- PINS ---------------- */
const int LED_PIN = D5;
const int BUTTON_PIN = D7;
const int DHT_PIN = D4;

/* ---------------- ETATS ---------------- */
int buttonState = LOW;
int ledState = LOW;
int dhtState = LOW;

/* -------------- DHT OBJECT -------------- */
float humidity;
float temperature;
float heatIndex;

/* -------- IINITIALIZE DHT SENSOR -------- */
DHT dht(DHT_PIN, DHTTYPE);

/* ---------------- TIMERS ---------------- */
long previousMillisBtn = 0;
long previousMillisDHT = 0;
long buttonInterval = 500;
long dhtInterval = 500;

/* ---------------- SERIAL ---------------- */
String inputString = "";
bool stringComplete = false;

/* ---------------- MENU ---------------- */
enum MenuState {
  MENU_MAIN,
  MENU_BUTTON,
  MENU_DHT
};

MenuState menu = MENU_MAIN;


/* =======================================================================
   SETUP
   ======================================================================= */
void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState);

  // pinMode(DHT_PIN, OUTPUT);
  // digitalWrite(DHT_PIN, dhtState);

  // Start DHT
  dht.begin();

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
  Serial.println("2 : Gérer l'acquisition du dht");
}

void showButtonMenu() {
  Serial.println("\n=== MENU BOUTON ===");
  Serial.println("Affichage des valeurs du bouton...");
  Serial.println("Commandes disponibles :");
  Serial.println("  BUTTONSET <ms>   : changer intervalle lecture");
  Serial.println("  RETURN           : retour au menu principal\n");
}

void showDHTMenu() {
  Serial.println("\n=== MENU DHT ===");
  Serial.println("Affichage des valeurs du dht...");
  Serial.println("Commandes disponibles :");
  Serial.println("  DHTSET <ms>   : changer intervalle lecture");
  Serial.println("  RETURN        : retour au menu principal\n");
}


/* =======================================================================
   LOOP
   ======================================================================= */
void loop() {

  // Lecture bouton seulement dans le menu bouton
  if (menu == MENU_BUTTON) {
    doEvery(buttonInterval, &previousMillisBtn, readButton);
  }

  if (menu == MENU_DHT) {
    doEvery(dhtInterval, &previousMillisDHT, readDHT);
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
   Fonction lecture dht
   ======================================================================= */
void readDHT() {

  humidity = dht.readHumidity();

  temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
  }
  else {
    // Compute heat index in Celsius (isFahreheit = false)
    heatIndex = dht.computeHeatIndex(temperature, humidity, false);
  
    Serial.print(F("Humidity: "));
    Serial.print(humidity);
    Serial.print(F("%  Temperature: "));
    Serial.print(temperature);
    Serial.print(F("°C "));
    Serial.print(F("  Heat index: "));
    Serial.println(heatIndex);
  }
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
      //Serial.println("Option 2 non encore implémentée.");
      menu = MENU_DHT;
      showDHTMenu();
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
    //Réponse à l'ereur
    Serial.println("Commande inconnue. Tapez RETURN pour revenir au menu principal.");
    return;
  }

    /* === COMMANDE MENU DHT === */
  if (menu == MENU_DHT) {

    // Retour au menu principal
    if (cmd == "RETURN") {
      menu = MENU_MAIN;
      showMainMenu();
      return;
    }

    // BUTTONSET <ms>
    if (cmd.startsWith("DHTSET")) {

      String valueStr = cmd.substring(6);
      valueStr.trim();
      long newInterval = valueStr.toInt();

      if (newInterval > 0) {
        dhtInterval = newInterval;
        Serial.print("Nouvel intervalle = ");
        Serial.print(dhtInterval);
        Serial.println(" ms");
      } else {
        Serial.println("Erreur: donnée non valide.");
      }
      return;
    }
    //Réponse à l'ereur
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


