/*********************************
* Simple led blinking 
* With Serial link at 115200 bauds
* Blink Onboard Led on D4
* Push Button on D7 (PullUp) => scrutation
* Board : ESP-D1  
* Author : O. Patrouix ESTIA
* Date : 29/09/2021
*********************************/

/* =======================================================================
   DECLARATIONS DE DEBUT DE PROGRAMME
   ======================================================================= */

// Include des classes
#include "ESP-D1_Functions.h"

// Include pour index.html
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

// Include des librairies locales
#include "ESP-D1_GPIO.h"
#include "DHT.h"
#define DHTTYPE DHT11


/* -------------- RESEAU -------------- */
// Paramètres du serveur en AP
ESP8266WebServer server(80);
const char* AP_ssid = "!!**IIoT4**!!";
const char* AP_password = "12345678";
IPAddress local_IP(192,168,4,29);
IPAddress gateway(192,168,4,99);
IPAddress subnet(255,255,255,0);
IPAddress AP_myIP;

// Paramètres du serveur en WS
const char* WS_ssid = "IIoT_A";
const char* WS_password = "IIoT_AS3";
IPAddress WS_local_IP(192,168,1,29);
IPAddress WS_gateway(192,168,1,12);
IPAddress WS_subnet(255,255,255,0);
IPAddress WS_myIP;


/* ---------------- PINS ---------------- */
const int LED_PIN = D5;
const int BUTTON_PIN = D7;
const int PIN_RELAIS = D1;  // pin relai selon câblage actuel
// GPIO à modifier plus tard pour la partie web
const int WEB0 =  GPIO_0;      
const int WEB2 =  GPIO_2;      


/* ---------------- ETATS ---------------- */
int buttonState = LOW;
int ledState = LOW;
bool etatRelais = false; //Variable bool pour ON/OFF relai
// Variables web (changer le nom GPIO)
int toggleRelais = LOW;
int StateWEB0 = HIGH;           
int StateWEB2 = LOW;     
// Etats Wifi
enum WifiModeState { 
  WIFI_MODE_AP, 
  WIFI_MODE_WS 
};
WifiModeState wifiMode = WIFI_MODE_AP;   // démarrage par défaut en AP
       

/* ------------- VARIABLES Actionneur ------------- */


/* ------ INTITIALISATION Actionneur relai ------ */


/* ---------------- TIMERS ---------------- */
long previousMillisBtn = 0;
long previousMillisDHT = 0;
long buttonInterval = 500;
long dhtInterval = 500;
long tempInterval = 500;
long humiInterval = 500;
// Timer web changer le nom plus tard
long previousMillis_2 = 0; 

/* ---------------- SERIAL ---------------- */
String inputString = "";
bool stringComplete = false;

/* ---------------- MENU ---------------- */
enum MenuState {
  MENU_MAIN,
  MENU_BUTTON,
  MENU_DHT,
  MENU_TEMP,
  MENU_HUMI,
  MENU_RELAI,
  MENU_WIFI
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

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  delay(300);
  Serial.println("\nBOOT ESP-D1 ready to work");

  showMainMenu();

  pinMode(WEB0, OUTPUT);
  digitalWrite(WEB0, HIGH);

  pinMode(WEB2, OUTPUT);
  digitalWrite(WEB2, LOW);

  pinMode(PIN_RELAIS, OUTPUT);

  // -------- WIFI MANAGEMENT --------
  applyWifiMode();


// -------- WEB HANDLERS --------

  // Page principale HTML
  server.on("/", []() {
    Serial.println(">>>WEB EVENT>>> Appel de l'index du site");
    server.send(200, "text/html", getPage());
  });

  // JSON : état du relais
  server.on("/relayState", []() {
    String json = "{";
    json += "\"state\":" + String(etatRelais ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  // Basculer le relais (toggle)
  server.on("/toggleRelay", []() {
    etatRelais = !etatRelais;
    digitalWrite(PIN_RELAIS, etatRelais ? HIGH : LOW);  // ou LOW/HIGH si relais actif à l'état bas

    Serial.print(">>>WEB EVENT>>> Nouveau state relais = ");
    Serial.println(etatRelais ? "ON" : "OFF");

    String json = "{";
    json += "\"state\":" + String(etatRelais ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
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
  Serial.println("3 : Visualiser et Gérer l'accès wifi");
  Serial.println("4 : Visualiser et Gérer l'aquisition temp");
  Serial.println("5 : Visualiser et Gérer l'aquisition humi");
  Serial.println("6 : Visualiser et Gérer l'activation du relai");
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

void showWifiMenu() {
  Serial.println("\n=== MENU WIFI ===");
  Serial.print("Mode actuel : ");
  Serial.println(wifiMode == WIFI_MODE_AP ? "AP" : "STA");

  Serial.println("Commandes disponibles :");
  Serial.println("       AP        : passer en Access Point");
  Serial.println("       STA       : passer en Station");
  Serial.println("  RETURN ou MENU : retour au menu principal\n");
}

void showTempMenu() {
  Serial.println("\n=== MENU Temp ===");
  Serial.println("Affichage des valeurs du temp...");
  Serial.println("Commandes disponibles :");
  Serial.println("  RETURN ou MENU  : retour au menu principal\n");
}

void showHumiMenu() {
  Serial.println("\n=== MENU Temp ===");
  Serial.println("Affichage des valeurs du humi...");
  Serial.println("Commandes disponibles :");
  Serial.println("  RETURN ou MENU  : retour au menu principal\n");
}

void showRelaiMenu() {
  Serial.println("\n=== MENU RELAI ===");
  Serial.print("Mode actuel : ");
  Serial.println(etatRelais? "ON" : "OFF");

  Serial.println("Commandes disponibles :");
  Serial.println("       ON        : Activer relai");
  Serial.println("       OFF       : Désactiver relai");
  Serial.println("  RETURN ou MENU : retour au menu principal\n");
}



/* =======================================================================
   LOOP
   ======================================================================= */
void loop() {

  // Lecture bouton seulement dans le menu bouton
  if (menu == MENU_BUTTON) {
    doEvery(buttonInterval, &previousMillisBtn, readButton);
  }

  // if (menu == MENU_DHT) {
  //   doEvery(dhtInterval, &previousMillisDHT, readDHT);
  // }

  // if (menu == MENU_TEMP) {
  //   doEvery(tempInterval, &previousMillisDHT, readDHT);
  // }
  // if (menu == MENU_HUMI) {
  //   doEvery(humiInterval, &previousMillisDHT, readDHT);
  // }

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


/* =======================================================================
   Analyse des commandes utilisateur
   ======================================================================= */
void parseCommand(String cmd) {

  cmd.trim();

  /* --- COMMANDE MENU PRINCIPAL --- */
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
    if (cmd == "3") {
      menu = MENU_WIFI;
      showWifiMenu();
      return;
    }
    if (cmd == "4") {
      menu = MENU_TEMP;
      showTempMenu();
      return;
    }
    if (cmd == "5") {
      menu = MENU_HUMI;
      showHumiMenu();
      return;
    }
    if (cmd == "6") {
      menu = MENU_RELAI;
      showRelaiMenu();
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
  /* === COMMANDE MENU WIFI === */
  if (menu == MENU_WIFI) {

    if (cmd.equalsIgnoreCase("return") || cmd.equalsIgnoreCase("menu") || cmd.equalsIgnoreCase("stop")) {
      menu = MENU_MAIN;
      showMainMenu();
      return;
    }

    if (cmd.equalsIgnoreCase("AP")) {
      wifiMode = WIFI_MODE_AP;
      applyWifiMode();
      Serial.println("Mode AP activé.");
      showWifiMenu();
      return;
    }

    if (cmd.equalsIgnoreCase("STA")) {
      wifiMode = WIFI_MODE_WS;
      applyWifiMode();
      //Serial.println("Mode STA activé.");
      showWifiMenu();
      return;
    }

    Serial.println("Commande inconnue.");
    showWifiMenu();
    return;
  }

    /* === COMMANDE MENU RELAI === */
  if (menu == MENU_RELAI) {

    if (cmd.equalsIgnoreCase("return") || cmd.equalsIgnoreCase("menu") || cmd.equalsIgnoreCase("stop")) {
      menu = MENU_MAIN;
      showMainMenu();
      return;
    }

    if (cmd.equalsIgnoreCase("ON")) {
      toggleRelais = HIGH;
      digitalWrite(PIN_RELAIS, HIGH);
      Serial.println("Relai activé.");
      //showRelaiMenu();
      return;
    }

    if (cmd.equalsIgnoreCase("OFF")) {
      toggleRelais = LOW;
      digitalWrite(PIN_RELAIS, LOW);
      Serial.println("Relai désactivé.");
      //showRelaiMenu();
      return;
    }

    Serial.println("Commande inconnue.");
    showWifiMenu();
    return;
  }

}

/* =======================================================================
   Wifi mode State
   ======================================================================= */
void applyWifiMode() {

  if (wifiMode == WIFI_MODE_AP) {
    Serial.println("Basculer en mode ACCESS POINT...");
    
    WiFi.disconnect(true);
    delay(200);

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP(AP_ssid, AP_password);

    AP_myIP = WiFi.softAPIP();

    Serial.print("AP IP address: ");
    Serial.println(AP_myIP);
  }

  if (wifiMode == WIFI_MODE_WS) {

    Serial.println("Basculer en mode STATION...");

    WiFi.disconnect(true);
    delay(200);

    WiFi.mode(WIFI_STA);
    WiFi.config(WS_local_IP, WS_gateway, WS_subnet);
    WiFi.begin(WS_ssid, WS_password);

    Serial.print("Connexion au réseau STA...");

    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 50) {
      delay(200);
      Serial.print(".");
      timeout++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      WS_myIP = WiFi.localIP();
      Serial.print("Connecté ! IP : ");
      Serial.println(WS_myIP);
    } 
    else {
      Serial.println("Échec de connexion STA, Retour mode AP");
      wifiMode = WIFI_MODE_AP;
      applyWifiMode();
    }
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


