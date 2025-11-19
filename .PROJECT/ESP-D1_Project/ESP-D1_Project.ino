/*********************************
* Full IIoT programm
* Publisher Role
* With Serial link at 115200 bauds
* Board : ESP-D1  
* Original Author : O. Patrouix ESTIA
* Student : Thiabud DANEDE
* Date : 19/11/2025
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

/* ---------------- CONFIG MQTT ---------------- */
const char* mqtt_server = "192.168.1.2";
const char* mqtt_topic  = "Greg";
const char* mqtt_user   = "usr10";
const char* mqtt_pass   = "usr10";
const char* clientID    = "Greg";


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
WifiModeState wifiMode = WIFI_MODE_WS;   // démarrage par défaut en AP
       

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
// Dernière tentative de reconnection
long previousReconnectAttempt = 0;
// Intervalle de tentative de reconnection
const long reconnectInterval = 30000; 
// Toggle de la reconnexion automatique
bool autoReconnectEnabled = true;  



/* ---------------- STILL ALIVE ---------------- */
long previousMillisAlive = 0;
long aliveInterval = 1000;   // par défaut AP

// Intervalles de clignotement selon le mode
const long aliveIntervalAP  = 400;   
const long aliveIntervalWS  = 1000;  
const long aliveIntervalSER = 150;  


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

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);


/* =======================================================================
   SETUP
   ======================================================================= */
void setup() {
  // Affectation de la led
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState);

  // Lancement du capteur
  dht.begin();

  // Activation du moniteur série
  Serial.begin(115200);
  delay(300);
  Serial.println("\nBOOT ESP-D1 ready to work");

  // Affichage du menu
  showMainMenu();

  // Application du mode wifi
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
   LOOP
   ======================================================================= */
void loop() {

  // Détection immédiate de perte WS
  if (wifiMode == WIFI_MODE_WS && WiFi.status() != WL_CONNECTED) {
      Serial.println("[WiFi] Perte WS détectée → bascule en AP");
      wifiMode = WIFI_MODE_AP;
      applyWifiMode();
  }

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

  // Verification continue de l'état de l'AP
  checkAP();
  // Tentative de reconnection périodique (30 secondes contrôlées par reconnectInterval)
  autoReconnectWS();

  //Handle du site web
  server.handleClient();

  // Handle du MQTT
  mqttClient.loop();
}


/* =======================================================================
   Fonctions Still Alive
   ======================================================================= */
// Acquisition de l'intervale à laquelle clignoter
long getAliveInterval() {

  if (wifiMode == WIFI_MODE_WS) {
    // Mode Station pour vérifier si réellement connecté
    if (WiFi.status() == WL_CONNECTED)
      return 3000;  // connecté WS
    else
      return 100;   // WS demandé mais pas connecté
  }

  if (wifiMode == WIFI_MODE_AP) {
    return 1000;
  }
  return 100;
}

// Appel du clignotement de la led en fonction de l'état du système
void stillAlive() {
  static bool led = false;
  // Contrôle du changement d'état de la led
  led = !led;
  digitalWrite(LED_PIN, led ? HIGH : LOW);
}


/* =======================================================================
   Analyse des commandes utilisateur
   ======================================================================= */
void parseCommand(String cmd) {

  cmd.trim();

  /* --- COMMANDE MENU PRINCIPAL --- */
  if (menu == MENU_MAIN) {
    // Lance le défilé du DHT
    if (cmd == "1") {
      menu = MENU_DHT;
      showDHTMenu();
      return;
    }
    // Affiche le menu WiFi
    if (cmd == "2") {
      menu = MENU_WIFI;
      showWifiMenu();
      return;
    }
    Serial.println("Commande inconnue.");
    showMainMenu();
    return;
  }

  /* --- COMMANDE MENU DHT --- */
  if (menu == MENU_DHT) {
    // Retour au menu principal
    if (cmd.equalsIgnoreCase("return") || cmd.equalsIgnoreCase("menu") || cmd.equalsIgnoreCase("stop")) {
    menu = MENU_MAIN;
    showMainMenu();
    return;
    }

    // DHTSET <ms>, l'utilisateur entre à côté de DHTSET sa nouvelle intervalle
    if (cmd.startsWith("DHTSET")) {
      String valueStr = cmd.substring(6);
      valueStr.trim();
      long newInterval = valueStr.toInt();

      // Valide seulement si la requête est > 0
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
    Serial.println("Commande inconnue. Tapez RETURN ou MENU pour revenir au menu principal.");
    return;
  }
  /* === COMMANDE MENU WIFI === */
  if (menu == MENU_WIFI) {
    // Commande de retour au menu
    if (cmd.equalsIgnoreCase("return") || cmd.equalsIgnoreCase("menu") || cmd.equalsIgnoreCase("stop")) {
      menu = MENU_MAIN;
      showMainMenu();
      return;
    }
    //Lance l'état Access Point
    if (cmd.equalsIgnoreCase("AP")) {
      wifiMode = WIFI_MODE_AP;
      applyWifiMode();
      return;
    }
    // Lance l'état Work Station
    if (cmd.equalsIgnoreCase("WS")) {
      wifiMode = WIFI_MODE_WS;
      applyWifiMode();
      return;
    }
    // Force l'état KO
    if (cmd.equalsIgnoreCase("KILL")) {
      wifiMode = WIFI_MODE_KO;
      Serial.println("Mode WiFi mis en carafe !");
      applyWifiMode();
    return;
    }

    // Toggle de la reconnexion automatique
    if (cmd.equalsIgnoreCase("RECO")) {
      autoReconnectEnabled = !autoReconnectEnabled;  // TOGGLE
      Serial.print("Reconnexion automatique : ");
      Serial.println(autoReconnectEnabled ? "ACTIVÉE" : "DÉSACTIVÉE");
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

  /* --- Mode AP --- */
  if (wifiMode == WIFI_MODE_AP) {
    Serial.println("\n[WiFi] Passage en mode ACCESS POINT...");

    WiFi.disconnect(true);
    delay(200);

    WiFi.mode(WIFI_AP);

    bool apOK = WiFi.softAP(AP_ssid, AP_password);
    WiFi.softAPConfig(local_IP, gateway, subnet);

    AP_myIP = WiFi.softAPIP();

    if (!apOK || AP_myIP.toString() == "0.0.0.0") {
      Serial.println("[WiFi] ERREUR : Impossible de demarrer AP !");
      Serial.println("[WiFi] Passage en mode KO (fallback)...");
      wifiMode = WIFI_MODE_KO;
      return;  // PAS de recursion — autoReconnectWS() gérera la suite
    }

    Serial.print("[WiFi] AP actif, IP = ");
    Serial.println(AP_myIP);
    return;
  }

  /* --- Mode AP --- */
  if (wifiMode == WIFI_MODE_WS) {
    Serial.println("\n[WiFi] Passage en mode STATION...");

    WiFi.disconnect(true);
    delay(200);

    WiFi.mode(WIFI_STA);
    WiFi.config(WS_local_IP, WS_gateway, WS_subnet);
    WiFi.begin(WS_ssid, WS_password);

    Serial.print("[WiFi] Connexion au réseau WS...");

    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 50) {
      delay(200);
      Serial.print(".");
      timeout++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {

      WS_myIP = WiFi.localIP();
      Serial.print("[WiFi] Connecté ! IP = ");
      Serial.println(WS_myIP);

      // Connexion MQTT
      connectMQTT();
      return;
    }

    // ---- ÉCHEC WS → AP ----
    Serial.println("[WiFi] Echec WS → bascule en AP...");
    wifiMode = WIFI_MODE_AP;
    applyWifiMode();   // OK ici car simple bascule WS→AP
    return;
  }

  // Mode KO
  if (wifiMode == WIFI_MODE_KO) {
    Serial.println("\n[WiFi] Mode KO : aucun WiFi actif.");

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    // autoReconnectWS() prendra le relais pour tenter WS puis AP
    return;
  }
}


/* =======================================================================
   Vérification continuelle de l'AP
   ======================================================================= */
void checkAP() {
  if (wifiMode == WIFI_MODE_AP) {

    if (WiFi.softAPIP().toString() == "0.0.0.0") {
      Serial.println("Wifi AP perdu ! Redémarrage du mode AP...");
      wifiMode = WIFI_MODE_AP;   
      applyWifiMode();          
    }
  }
}


/* =======================================================================
   Tentatives de reconnection automatique
   ======================================================================= */
void autoReconnectWS() {
  // Si RECO = OFF, pas de reconnection automatique
  if (!autoReconnectEnabled) 
    return;
  // Si déjà en WS, pas de reconnection automatique
  if (wifiMode == WIFI_MODE_WS)
    return; 

  unsigned long now = millis();

  // Tenter seulement toutes les X secondes
  if (now - previousReconnectAttempt < reconnectInterval)
    return;

  previousReconnectAttempt = now;

  Serial.println("\n[Auto] Tentative de reconnection au WS...");

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);

  WiFi.config(WS_local_IP, WS_gateway, WS_subnet);

  WiFi.begin(WS_ssid, WS_password);

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 30) {
    delay(100);
    Serial.print(".");
    timeout++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[Auto] Reconnexion WS réussie !");

    wifiMode = WIFI_MODE_WS;
    WS_myIP = WiFi.localIP();

    Serial.print("Nouvelle IP WS : ");
    Serial.println(WS_myIP); 

    connectMQTT();
  }
  else {
    Serial.println("[Auto] Échec WS, reconnexion AP.");
    wifiMode = WIFI_MODE_AP;
    applyWifiMode();
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


