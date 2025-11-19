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

// Include de la librairie MQTT
#include <PubSubClient.h>


/* -------------- RESEAU -------------- */
// Paramètres du serveur en AP
ESP8266WebServer server(80);
const char* AP_ssid = "!!?AA?!!";
const char* AP_password = "12345678";
IPAddress local_IP(192,168,4,99);
IPAddress gateway(192,168,4,99);
IPAddress subnet(255,255,255,0);
IPAddress AP_myIP;

// Paramètres du serveur en WS
const char* WS_ssid = "IIoT_A";
const char* WS_password = "IIoT_AS3";
IPAddress WS_local_IP(192,168,1,30);
IPAddress WS_gateway(192,168,1,12);
IPAddress WS_subnet(255,255,255,0);
IPAddress WS_myIP;


/* ---------------- PINS ---------------- */
const int LED_PIN = D5;
//const int BUTTON_PIN = D7;
const int DHT_PIN = D4;
// GPIO à modifier plus tard pour la partie web
const int WEB0 =  GPIO_0;      
const int WEB2 =  GPIO_2;      


/* ---------------- ETATS ---------------- */
//int buttonState = LOW;
int ledState = LOW;
int dhtState = LOW;
// Variables web (changer le nom GPIO)
int StateWEB0 = HIGH;           
int StateWEB2 = LOW;     
// Etats Wifi
enum WifiModeState { 
  WIFI_MODE_AP, 
  WIFI_MODE_WS,
  WIFI_MODE_KO 
};
WifiModeState wifiMode = WIFI_MODE_AP;   // démarrage par défaut en AP
       

/* ------------- VARIABLES DHT ------------- */
float humidity;
float temperature;
float heatIndex;

/* ------ INTITIALISATION DHT SENSOR ------ */
DHT dht(DHT_PIN, DHTTYPE);

/* ---------------- TIMERS ---------------- */
//long previousMillisBtn = 0;
long previousMillisDHT = 0;
//long buttonInterval = 500;
long dhtInterval = 500;
// Timer web changer le nom plus tard
long previousMillis_2 = 0; 
// Contrôle etat MQTT
long lastReconnectAttempt = 0;


/* ---------------- STILL ALIVE ---------------- */
long previousMillisAlive = 0;
long aliveInterval = 1000;   // par défaut AP

// Intervalles de clignotement selon le mode
const long aliveIntervalAP  = 400;   // mode AP  -> clignotement moyen
const long aliveIntervalWS  = 1000;  // mode WS  -> clignotement lent
const long aliveIntervalSER = 150;   // mode "sécurité" éventuel (on l’utilisera plus tard)


/* ---------------- SERIAL ---------------- */
String inputString = "";
bool stringComplete = false;

/* ---------------- MENU ---------------- */
enum MenuState {
  MENU_MAIN,
  MENU_DHT,
  MENU_WIFI
};

MenuState menu = MENU_MAIN;

/* ---------------- CONFIG MQTT ---------------- */
const char* mqtt_server = "192.168.1.2";
const char* mqtt_topic  = "Greg";
const char* mqtt_user   = "usr10";
const char* mqtt_pass   = "usr10";
const char* clientID    = "Greg";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);


/* =======================================================================
   WEB HANDLES
   ======================================================================= */

void handleRoot() {

  server.send(200, "text/html", getPage());
  digitalWrite(WEB0, HIGH);
}

void handleOpen() {

  server.send(200, "text/html", getPage());

  StateWEB0 = LOW;
  digitalWrite(WEB0, StateWEB0);
  delay(500);
  StateWEB0 = HIGH;
  digitalWrite(WEB0, StateWEB0);
}

void handleClose() {

  server.send(200, "text/html", getPage());

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

  Serial.begin(115200);
  delay(300);
  Serial.println("\nBOOT ESP-D1 ready to work");

  showMainMenu();

    // LED "Still Alive"
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);


  // -------- WIFI MANAGEMENT --------
  applyWifiMode();


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
      mqttPublish("DHT START");
      server.send(200, "text/plain", "DHT acquisition started");
  });

  server.on("/stopDHT", []() {
      menu = MENU_MAIN;
      Serial.println(">>>WEB EVENT>>> DHT STOP depuis le site");
      mqttPublish("DHT STOP");
      server.send(200, "text/plain", "DHT acquisition stopped");
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
  Serial.println("1 : Visualiser et gérer l’acquisition du DHT");
  Serial.println("2 : Visualiser et gérer l’accès WiFi");

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
  Serial.println(wifiMode == WIFI_MODE_AP ? "AP" : "WS");

  Serial.println("Commandes disponibles :");
  Serial.println("       AP        : passer en Access Point");
  Serial.println("       WS        : passer en Station");
  Serial.println("  RETURN ou MENU : retour au menu principal\n");
}



/* =======================================================================
   LOOP
   ======================================================================= */
void loop() {
  if (menu == MENU_DHT) {
    doEvery(dhtInterval, &previousMillisDHT, readDHT);
  }

  
  // Still Alive (clignotement selon le mode WiFi)
  doEvery(getAliveInterval(), &previousMillisAlive, stillAlive);
  
  // Lecture série
  serialEvent();

  if (stringComplete) {
    parseCommand(inputString);
    inputString = "";
    stringComplete = false;
  }

  //Handle du site web
  server.handleClient();

  // Handle du MQTT
  mqttClient.loop();
}


/* =======================================================================
   Fonctions Still Alive
   ======================================================================= */
long getAliveInterval() {

  if (wifiMode == WIFI_MODE_WS) {
    // Mode Station : vérifier si réellement connecté
    if (WiFi.status() == WL_CONNECTED)
      return 3000;  // connecté WS
    else
      return 100;   // WS demandé mais pas connecté
  }

  if (wifiMode == WIFI_MODE_AP) {
    // Mode AP : toujours ok
    return 1000;
  }

  // fallback sécurité
  return 100;
}

void stillAlive() {
  static bool led = false;

  led = !led;
  digitalWrite(LED_PIN, led ? HIGH : LOW);
}


/* =======================================================================
   Fonction lecture bouton
   ======================================================================= */
// void readButton() {
//   int state = digitalRead(BUTTON_PIN);

//   if (state == LOW) {
//     buttonState = LOW;
//     ledState = LOW;
//   } else {
//     buttonState = HIGH;
//     ledState = HIGH;
//   }

//   digitalWrite(LED_PIN, ledState);

//   Serial.print("Bouton pressé : ");
//   Serial.println(buttonState);
// }

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

    // MQTT publication 
    String payload = "{\"humidity\":" + String(humidity,1) + ",\"temperature\":" + String(temperature,1) + ",\"heatIndex\":" + String(heatIndex,1) + ",\"signature\":\"Thibaud\"}";
    mqttPublish(payload);
  }
}


/* =======================================================================
   Analyse des commandes utilisateur
   ======================================================================= */
void parseCommand(String cmd) {

  cmd.trim();

  /* --- COMMANDE MENU PRINCIPAL --- */
  if (menu == MENU_MAIN) {
    // if (cmd == "1") {
    //   menu = MENU_BUTTON;
    //   showButtonMenu();
    //   return;
    // }
    if (cmd == "1") {
      //Serial.println("Option 2 non encore implémentée.");
      menu = MENU_DHT;
      showDHTMenu();
      return;
    }
    if (cmd == "2") {
      menu = MENU_WIFI;
      showWifiMenu();
      return;
    }
    Serial.println("Commande inconnue.");
    showMainMenu();
    return;
  }


  /* === COMMANDE MENU BOUTON === */
  // if (menu == MENU_BUTTON) {

  //   // Retour au menu principal
  //   if (cmd.equalsIgnoreCase("return") || cmd.equalsIgnoreCase("menu") || cmd.equalsIgnoreCase("stop")) {
  //   menu = MENU_MAIN;
  //   showMainMenu();
  //   return;
  //   }


  //   // BUTTONSET <ms>
  //   if (cmd.startsWith("BUTTONSET")) {

  //     String valueStr = cmd.substring(9);
  //     valueStr.trim();
  //     long newInterval = valueStr.toInt();

  //     if (newInterval > 0) {
  //       buttonInterval = newInterval;
  //       Serial.print("Nouvel intervalle = ");
  //       Serial.print(buttonInterval);
  //       Serial.println(" ms");
  //     } else {
  //       Serial.println("Erreur: donnée non valide.");
  //     }
  //     return;
  //   }
  //   //Réponse à l'ereur
  //   Serial.println("Commande inconnue. Tapez RETURN pour revenir au menu principal.");
  //   return;
  // }

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
      return;
    }

    if (cmd.equalsIgnoreCase("WS")) {
      wifiMode = WIFI_MODE_WS;
      applyWifiMode();
      return;
    }

    if (cmd.equalsIgnoreCase("KILL")) {
      wifiMode = WIFI_MODE_KO;
      Serial.println("Mode WiFi mis en carafe !");
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

  // Mode AP
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

  // Mode WS (également appelé STA)
  if (wifiMode == WIFI_MODE_WS) {

    Serial.println("Basculer en mode STATION...");

    WiFi.disconnect(true);
    delay(200);

    WiFi.mode(WIFI_STA);
    WiFi.config(WS_local_IP, WS_gateway, WS_subnet);
    WiFi.begin(WS_ssid, WS_password);

    Serial.print("Connexion au réseau WS...");

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

      //Connexion au MQTT lors de l'exécution de la connexion au réseau IIoT_A
      if (mqttClient.state() != -2) {
        connectMQTT();
      } 
    }
    else {
      Serial.println("Échec de connexion WS, Retour mode AP");
      wifiMode = WIFI_MODE_AP;
      applyWifiMode();
    }
  }
  // Mode KO
  if (wifiMode == WIFI_MODE_KO) {
    Serial.println("WiFi volontairement en carafe. Aucun WiFi actif.");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
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
   Connexion au MQTT
   ======================================================================= */
void connectMQTT() {
  if (wifiMode != WIFI_MODE_WS) {
    Serial.println("MQTT désactivé (mode AP).");
    return;
  }

  mqttClient.setServer(mqtt_server, 1883);

  Serial.print("Connexion au serveur MQTT... ");

  if (mqttClient.connect(clientID, mqtt_user, mqtt_pass)) {
    Serial.println("OK !");
    Serial.print("Connecté au Topic ");
    Serial.print(mqtt_topic);
    Serial.print(" en tant que ");
    Serial.println(mqtt_user);

  } else {
    Serial.print("Échec, code = ");
    Serial.println(mqttClient.state());
  }
}


/* =======================================================================
   Publication sur le MQTT
   ======================================================================= */
void mqttPublish(String payload) {
  if (wifiMode != WIFI_MODE_WS) return;
  
  // Si le client n'est pas connecté, on gère la reconnexion
  if (!mqttClient.connected()) {
    long now = millis();
    // On ne tente de se reconnecter que toutes les 5 secondes
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Tentative de reconnexion
      connectMQTT(); 
    }
  }

  // Si après la tentative (ou si on l'était déjà), on est connecté, on publie
  if (mqttClient.connected()) {
    mqttClient.publish(mqtt_topic, payload.c_str());
    Serial.println("Envoi au MQTT " + payload);
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


