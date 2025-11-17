/*********************************
* Simple led blinking 
* With Serial link at 115200 bauds
* Blink Onboard Led on D4
* Push Button on D7 (PullUp) => scrutation
* Board : ESP-D1  
* Author : O. Patrouix ESTIA
* Date : 29/09/2021
*********************************/
// Include des classes
#include "ESP-D1_Functions.h"


// Include pour index.html
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>


// Paramètres du serveur
ESP8266WebServer server(80);
const char* ssid = "!!?AA?!!";
const char* password = "12345678";
IPAddress local_IP(192,168,4,99);
IPAddress gateway(192,168,4,99);
IPAddress subnet(255,255,255,0);
IPAddress myIP;

// Include des librairies locales
#include "ESP-D1_GPIO.h"
#include "DHT.h"
#define DHTTYPE DHT11

/* ---------------- PINS ---------------- */
const int LED_PIN = D5;
const int BUTTON_PIN = D7;
const int DHT_PIN = D4;
// GPIO à modifier plus tard pour la partie web
const int WEB0 =  GPIO_0;      
const int WEB2 =  GPIO_2;      

/* ---------------- ETATS ---------------- */
int buttonState = LOW;
int ledState = LOW;
int dhtState = LOW;
// Variables web (changer le nom GPIO)
int StateWEB0 = HIGH;           
int StateWEB2 = LOW;            
       

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
// Timer web changer le nom plus tard
long previousMillis_2 = 0; 

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
   WEB HANDLES
   ======================================================================= */
// To Do on http://192.168.4.XX/
// Set WEB0 to HIGH <=> relay opened
void handleRoot() {
  //server.send(200, "text/html", "<h1>You are connected</h1>");
  server.send(200, "text/html", getPage());
  digitalWrite(WEB0, HIGH);
}
// To Do on http://192.168.4.XX/open
// Set WEB0 to LOW for 500ms <=> relay closed
void handleOpen() {
  //server.send(200, "text/html", "<h1>Gate Opening</h1>");
  server.send(200, "text/html", getPage());
  // set the LED with the ledState of the variable:
  StateWEB0 = LOW;
  digitalWrite(WEB0, StateWEB0);
  delay(500);
  StateWEB0 = HIGH;
  digitalWrite(WEB0, StateWEB0);
}

// To Do on http://192.168.4.XX/close
// Set WEB0 to LOW for 500ms <=> relay closed
void handleClose() {
  //server.send(200, "text/html", "<h1>Gate Closing</h1>");
  server.send(200, "text/html", getPage());
  // set the LED with the ledState of the variable:
  StateWEB0 = LOW;
  digitalWrite(WEB0, StateWEB0);
  delay(500);
  StateWEB0 = HIGH;
  digitalWrite(WEB0, StateWEB0);
}

/* =======================================================================
   SETUP
   ======================================================================= */
void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState);

  dht.begin();
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  delay(300);
  Serial.println("\nBOOT ESP-D1 ready to work");

  showMainMenu();

  pinMode(WEB0, OUTPUT);
  digitalWrite(WEB0, HIGH);

  pinMode(WEB2, OUTPUT);
  digitalWrite(WEB2, LOW);

  // -------- WIFI ACCESS POINT --------
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);

  myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  // -------- WEB HANDLERS --------

  // Page principale HTML
  server.on("/", []() {
      Serial.println(">>>WEB EVENT>>> Appel de l'index du site");
      server.send(200, "text/html", getPage());
  });

  // Données JSON du DHT
  server.on("/dht", []() {
      server.send(200, "application/json", getDHTjson(humidity, temperature, heatIndex));
  });

  // Commandes contrôle DHT
  server.on("/startDHT", []() {
      menu = MENU_DHT;
      Serial.println(">>>WEB EVENT>>> DHT START depuis le site");
      server.send(200, "text/plain", "DHT acquisition started");
  });

  server.on("/stopDHT", []() {
      menu = MENU_MAIN;
      Serial.println(">>>WEB EVENT>>> DHT STOP depuis le site");
      server.send(200, "text/plain", "DHT acquisition stopped");
  });

  // Commandes contrôle bouton
  server.on("/startButton", []() {
      menu = MENU_BUTTON;
      Serial.println(">>>WEB EVENT>>> Bouton START depuis le site");
      server.send(200, "text/plain", "Button acquisition started");
  });

  server.on("/stopButton", []() {
      menu = MENU_MAIN;
      Serial.println(">>>WEB EVENT>>> Bouton STOP depuis le site");
      server.send(200, "text/plain", "Button acquisition stopped");
  });

  server.on("/dhtState", []() {
    String json = "{";
    json += "\"active\":" + String(menu == MENU_DHT ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/dhtInterval", []() {  
    String json = "{";
    json += "\"interval\":" + String(dhtInterval); 
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/setDHTinterval", []() {
    if (!server.hasArg("value")) {
        server.send(400, "text/plain", "Missing value");
        return;
    }
    int newVal = server.arg("value").toInt();
    if (newVal < 100) newVal = 100;  // protection

    dhtInterval = newVal;
    Serial.print(">>>WEB EVENT>>> Nouvelle intervale à ");
    Serial.print(newVal);
    Serial.println(" ms depuis le site");
    server.send(200, "text/plain", "OK");
  });



  server.begin();
  Serial.println("HTTP server started");
}



/* =======================================================================
   MENU AFFICHAGE
   ======================================================================= */
void showMainMenu() {
  Serial.println("\n======== MENU PRINCIPAL ========");
  Serial.println("Tapez dans l'invite de commande le numéro du paramètre que vous voulez régler");
  Serial.println("1 : Vidualiser et Gérer l'acquisition du bouton");
  Serial.println("2 : Visualiser et Gérer l'acquisition du dht");
}

void showButtonMenu() {
  Serial.println("\n=== MENU BOUTON ===");
  Serial.println("Affichage des valeurs du bouton...");
  Serial.println("Commandes disponibles :");
  Serial.println("  BUTTONSET <ms>   : changer intervalle lecture");
  Serial.println("  RETURN ou MENU   : retour au menu principal\n");
}

void showDHTMenu() {
  Serial.println("\n=== MENU DHT ===");
  Serial.println("Affichage des valeurs du dht...");
  Serial.println("Commandes disponibles :");
  Serial.println("  DHTSET <ms>     : changer intervalle lecture");
  Serial.println("  RETURN ou MENU  : retour au menu principal\n");
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

  //Handle
  server.handleClient();
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
    if (cmd.equalsIgnoreCase("return") || cmd.equalsIgnoreCase("menu") || cmd.equalsIgnoreCase("stop")) {
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
    if (cmd.equalsIgnoreCase("return") || cmd.equalsIgnoreCase("menu") || cmd.equalsIgnoreCase("stop")) {
    menu = MENU_MAIN;
    showMainMenu();
    return;
    }

    // DHTSET <ms>
    if (cmd.startsWith("DHTSET")) {

      String valueStr = cmd.substring(6);
      valueStr.trim();
      long newInterval = valueStr.toInt();

      if (newInterval > 0) {
        dhtInterval = newInterval;
        Serial.print("Nouvel intervalle = ");
        Serial.print(dhtInterval);
        Serial.println(" ms depuis la console");
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


