/*********************************
* Projet IIoT - Subscriber Relai + Web + MQTT
* Board : ESP-D1  
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

// MQTT
#include <PubSubClient.h>
#include <math.h>   // pour isnan()

// Include des librairies locales
#include "ESP-D1_GPIO.h"


/* -------------- RESEAU -------------- */
// Paramètres du serveur en AP (point d'accès local)
ESP8266WebServer server(80);
const char* AP_ssid     = "!!**IIoT4**!!";
const char* AP_password = "12345678";
IPAddress local_IP(192,168,4,29);
IPAddress gateway(192,168,4,99);
IPAddress subnet(255,255,255,0);
IPAddress AP_myIP;

// Paramètres du serveur en WS (mode HUB / Station)
const char* WS_ssid     = "IIoT_A";      // ou "robotique" selon la consigne
const char* WS_password = "IIoT_AS3";    // ou "robotiqueS3"
IPAddress WS_local_IP(192,168,1,29);
IPAddress WS_gateway(192,168,1,12);
IPAddress WS_subnet(255,255,255,0);
IPAddress WS_myIP;


/* ---------------- MQTT (SUBSCRIBER) ---------------- */
// ⚠️ A ADAPTER SELON TES IDENTIFIANTS ⚠️
const char* mqtt_server   = "192.168.1.2";
const char* mqtt_topic    = "Greg";   // ex: "Test" / "usrX" / etc.
const char* mqtt_username = "usr9";   // ex: "usrX"
const char* mqtt_password = "usr9";   // ex: "usrX"
const char* clientID      = "Ory";    // ex: "ESP8266_SubX"

WiFiClient    wifiClient;
PubSubClient  client(mqtt_server, 1883, wifiClient); // 1883 = port MQTT standard


/* ---------------- PINS ---------------- */
const int LED_PIN    = D5;
const int BUTTON_PIN = D7;
const int PIN_RELAIS = D1;  // pin relai selon câblage actuel

// GPIO à modifier plus tard pour la partie web
const int WEB0 = GPIO_0;   // LED ou sortie pour debug
const int WEB2 = GPIO_2;   // LED still alive


/* ---------------- ETATS ---------------- */
int  buttonState = LOW;
int  ledState    = LOW;
bool etatRelais  = false;   // Variable bool pour ON/OFF relai

// Variables web
int StateWEB0    = HIGH;           
int StateWEB2    = LOW;     

// Mesures reçues via MQTT (pas de vrai DHT sur ta carte)
float humidity    = NAN;
float temperature = NAN;
float heatIndex   = NAN;
bool  hasDhtData  = false;   // true après première réception MQTT

// Etats Wifi
enum WifiModeState { 
  WIFI_MODE_AP, 
  WIFI_MODE_WS 
};
WifiModeState wifiMode = WIFI_MODE_AP;   // démarrage par défaut en AP
       

/* ---------------- TIMERS ---------------- */
long previousMillisBtn = 0;
long previousMillisDHT = 0;
long buttonInterval    = 500;
long dhtInterval       = 1000;  // 1 s
long tempInterval      = 1000;
long humiInterval      = 1000;

// Timer LED still alive (WEB2)
long previousMillis_2  = 0; 
long intervalMillis_2  = 1000; // 1s clignotement lent

/* ---------------- SERIAL ---------------- */
String inputString  = "";
bool   stringComplete = false;

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
   ACTIONNEUR : fonction centrale pour le relai
   ======================================================================= */
void setRelai(bool on) {
  etatRelais = on;
  digitalWrite(PIN_RELAIS, etatRelais ? HIGH : LOW);  // inverse si relais actif niveau bas
  Serial.print("[ACTIONNEUR] Relai ");
  Serial.println(etatRelais ? "ON" : "OFF");
}


/* =======================================================================
   WEB HANDLES (anciens exemples, peu utilisés mais on garde)
   ======================================================================= */
// To Do on http://192.168.4.XX/
void handleRoot() {
  server.send(200, "text/html", getPage());
  digitalWrite(WEB0, HIGH);
}

// To Do on http://192.168.4.XX/open
void handleOpen() {
  server.send(200, "text/html", getPage());
  StateWEB0 = LOW;
  digitalWrite(WEB0, StateWEB0);
  delay(500);
  StateWEB0 = HIGH;
  digitalWrite(WEB0, StateWEB0);
}

// To Do on http://192.168.4.XX/close
void handleClose() {
  server.send(200, "text/html", getPage());
  StateWEB0 = LOW;
  digitalWrite(WEB0, StateWEB0);
  delay(500);
  StateWEB0 = HIGH;
  digitalWrite(WEB0, StateWEB0);
}


/* =======================================================================
   JSON helper pour extraire un float depuis le payload MQTT
   ======================================================================= */
float extractJsonFloat(const String& json, const String& key) {
  int idx = json.indexOf(key);
  if (idx == -1) return NAN;

  idx = json.indexOf(':', idx);
  if (idx == -1) return NAN;

  int start = idx + 1;
  // sauter espaces et guillemets
  while (start < (int)json.length() && (json[start] == ' ' || json[start] == '\"')) {
    start++;
  }

  int end = start;
  while (end < (int)json.length()) {
    char c = json[end];
    if ((c >= '0' && c <= '9') || c == '.' || c == '-' ) {
      end++;
    } else {
      break;
    }
  }

  if (end <= start) return NAN;

  String numStr = json.substring(start, end);
  return numStr.toFloat();
}


/* =======================================================================
   MQTT CALLBACK & CONNEXION
   ======================================================================= */

// Callback MQTT à chaque message reçu
void ReceivedMessage(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  // Cas 1 : message "B" comme dans l'exemple -> toggle relai
  if (length == 1 && (char)payload[0] == 'B') {
    setRelai(!etatRelais);
    return;
  }

  // Cas 2 : JSON avec humidity/temperature/heatindex
  float h  = extractJsonFloat(msg, "humidity");
  float t  = extractJsonFloat(msg, "temperature");
  float hi = extractJsonFloat(msg, "heatindex");

  if (!isnan(h) && !isnan(t)) {
    humidity    = h;
    temperature = t;
    if (!isnan(hi)) {
      heatIndex = hi;
    }
    hasDhtData = true;
  }
  // Pas de Serial.print ici -> aucune ligne "[MQTT] ..." dans la console
}

// Connexion au broker + abonnement au topic (copié de l'exemple)
bool Connect() {
  if (client.connect(clientID, mqtt_username, mqtt_password)) {
    client.subscribe(mqtt_topic);
    Serial.print("[MQTT] Connecté, abonné à : ");
    Serial.println(mqtt_topic);
    return true;
  } else {
    Serial.print("[MQTT] Echec connexion, rc=");
    Serial.println(client.state());
    return false;
  }
}


/* =======================================================================
   Blink Led (Still Alive) comme l'exemple
   ======================================================================= */
void BlinkLed(int GPIO, int *StateGPIO, long *previousMillis, long intervalMillis){
  unsigned long currentMillis = millis();
 
  if(currentMillis - *previousMillis > intervalMillis) {
    *previousMillis = currentMillis;   

    if (*StateGPIO == HIGH) {
      *StateGPIO = LOW;
    } else {
      *StateGPIO = HIGH;
    }
    digitalWrite(GPIO, *StateGPIO);
  }
}


/* =======================================================================
   PROTOTYPES
   ======================================================================= */
void showMainMenu();
void showButtonMenu();
void showDHTMenu();
void showWifiMenu();
void showTempMenu();
void showHumiMenu();
void showRelaiMenu();
void applyWifiMode();
void readButton();
void readDHT();
void parseCommand(String cmd);
void serialEvent();
void doEvery(long intervalMillis, long *previousMillis, void (*action)());


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
  setRelai(false);   // relais au repos

  // -------- WIFI MANAGEMENT (AP ou STA suivant wifiMode) --------
  applyWifiMode();

  // -------- MQTT : définition du callback --------
  client.setCallback(ReceivedMessage);

  // -------- WEB HANDLERS modernes (pour ta page HTML) --------

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

  // Basculer le relais (toggle) via la page web
  server.on("/toggleRelay", []() {
    setRelai(!etatRelais);

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
   MENUS AFFICHAGE
   ======================================================================= */
void showMainMenu() {
  Serial.println("\n======== MENU PRINCIPAL ========");
  Serial.println("1 : Visualiser et Gérer l'acquisition du bouton");
  Serial.println("2 : Visualiser et Gérer l'acquisition du dht (Temp + Humi via MQTT)");
  Serial.println("3 : Visualiser et Gérer l'accès wifi");
  Serial.println("4 : Visualiser et Gérer l'aquisition temp (via MQTT)");
  Serial.println("5 : Visualiser et Gérer l'aquisition humi (via MQTT)");
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
  Serial.println("Affichage des valeurs du DHT (Temp + Humi via MQTT)...");
  Serial.println("Commandes disponibles :");
  Serial.println("  DHTSET <ms>     : changer intervalle affichage");
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
  Serial.println("Affichage de la TEMPERATURE seulement (via MQTT)...");
  Serial.println("Commandes disponibles :");
  Serial.println("  TEMPSET <ms>   : changer intervalle affichage");
  Serial.println("  RETURN ou MENU : retour au menu principal\n");
}

void showHumiMenu() {
  Serial.println("\n=== MENU Humi ===");
  Serial.println("Affichage de l'HUMIDITE seulement (via MQTT)...");
  Serial.println("Commandes disponibles :");
  Serial.println("  HUMISET <ms>   : changer intervalle affichage");
  Serial.println("  RETURN ou MENU : retour au menu principal\n");
}

void showRelaiMenu() {
  Serial.println("\n=== MENU RELAI ===");
  Serial.print("Etat actuel : ");
  Serial.println(etatRelais ? "ON" : "OFF");

  Serial.println("Commandes disponibles :");
  Serial.println("       ON        : Activer relai");
  Serial.println("       OFF       : Désactiver relai");
  Serial.println("  RETURN ou MENU : retour au menu principal\n");
}


/* =======================================================================
   LOOP
   ======================================================================= */
void loop() {

  // LED Still Alive sur WEB2 (clignotement lent façon exemple)
  BlinkLed(WEB2, &StateWEB2, &previousMillis_2, intervalMillis_2);

  // Lecture bouton seulement dans le menu bouton
  if (menu == MENU_BUTTON) {
    doEvery(buttonInterval, &previousMillisBtn, readButton);
  }

  // Affichage DHT suivant le menu :
  // - MENU_DHT  : temp + humi
  // - MENU_TEMP : seulement temp
  // - MENU_HUMI : seulement humi
  if (menu == MENU_DHT) {
    doEvery(dhtInterval, &previousMillisDHT, readDHT);
  }
  if (menu == MENU_TEMP) {
    doEvery(tempInterval, &previousMillisDHT, readDHT);
  }
  if (menu == MENU_HUMI) {
    doEvery(humiInterval, &previousMillisDHT, readDHT);
  }

  // Lecture série
  serialEvent();

  if (stringComplete) {
    parseCommand(inputString);
    inputString = "";
    stringComplete = false;
  }

  // Serveur web
  server.handleClient();

  // MQTT : seulement en mode Station (HUB) et WiFi connecté
  if (wifiMode == WIFI_MODE_WS && WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      Connect();
    }
    client.loop();
  }
}


/* =======================================================================
   Fonction lecture bouton
   ======================================================================= */
void readButton() {
  int state = digitalRead(BUTTON_PIN);

  if (state == LOW) {
    buttonState = LOW;
    ledState    = LOW;
  } else {
    buttonState = HIGH;
    ledState    = HIGH;
  }

  digitalWrite(LED_PIN, ledState);

  Serial.print("Bouton pressé : ");
  Serial.println(buttonState);
}


/* =======================================================================
   Fonction "readDHT" -> AFFICHER les valeurs MQTT selon le menu
   ======================================================================= */
void readDHT() {
  if (!hasDhtData) {
    Serial.println("Aucune donnée DHT reçue via MQTT pour le moment.");
    return;
  }

  if (menu == MENU_DHT) {
    // Temp + Humi
    Serial.print("Humidite : ");
    Serial.print(humidity, 1);
    Serial.print(" %  |  ");

    Serial.print("Temperature : ");
    Serial.print(temperature, 1);
    Serial.println(" °C");
  }
  else if (menu == MENU_TEMP) {
    // Seulement température
    Serial.print("Temperature : ");
    Serial.print(temperature, 1);
    Serial.println(" °C");
  }
  else if (menu == MENU_HUMI) {
    // Seulement humidité
    Serial.print("Humidite : ");
    Serial.print(humidity, 1);
    Serial.println(" %");
  }
}


/* =======================================================================
   Analyse des commandes utilisateur
   ======================================================================= */
void parseCommand(String cmd) {

  cmd.trim();

  /* --- COMMANDE MENU PRINCIPAL --- */
  if (menu == MENU_MAIN) {
    if (cmd == "1") { menu = MENU_BUTTON; showButtonMenu(); return; }
    if (cmd == "2") { menu = MENU_DHT;    showDHTMenu();    return; }
    if (cmd == "3") { menu = MENU_WIFI;   showWifiMenu();   return; }
    if (cmd == "4") { menu = MENU_TEMP;   showTempMenu();   return; }
    if (cmd == "5") { menu = MENU_HUMI;   showHumiMenu();   return; }
    if (cmd == "6") { menu = MENU_RELAI;  showRelaiMenu();  return; }

    Serial.println("Commande inconnue.");
    showMainMenu();
    return;
  }

  /* === COMMANDE MENU BOUTON === */
  if (menu == MENU_BUTTON) {

    if (cmd.equalsIgnoreCase("return") || cmd.equalsIgnoreCase("menu") || cmd.equalsIgnoreCase("stop")) {
      menu = MENU_MAIN;
      showMainMenu();
      return;
    }

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

  /* === COMMANDE MENU DHT === */
  if (menu == MENU_DHT) {

    if (cmd.equalsIgnoreCase("return") || cmd.equalsIgnoreCase("menu") || cmd.equalsIgnoreCase("stop")) {
      menu = MENU_MAIN;
      showMainMenu();
      return;
    }

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

    Serial.println("Commande inconnue. Tapez RETURN pour revenir au menu principal.");
    return;
  }

  /* === COMMANDE MENU TEMP === */
  if (menu == MENU_TEMP) {

    if (cmd.equalsIgnoreCase("return") || cmd.equalsIgnoreCase("menu") || cmd.equalsIgnoreCase("stop")) {
      menu = MENU_MAIN;
      showMainMenu();
      return;
    }

    if (cmd.startsWith("TEMPSET")) {
      String valueStr = cmd.substring(7);
      valueStr.trim();
      long newInterval = valueStr.toInt();

      if (newInterval > 0) {
        tempInterval = newInterval;
        Serial.print("Nouvel intervalle Temp = ");
        Serial.print(tempInterval);
        Serial.println(" ms");
      } else {
        Serial.println("Erreur: donnée non valide.");
      }
      return;
    }

    Serial.println("Commande inconnue. Tapez RETURN pour revenir au menu principal.");
    return;
  }

  /* === COMMANDE MENU HUMI === */
  if (menu == MENU_HUMI) {

    if (cmd.equalsIgnoreCase("return") || cmd.equalsIgnoreCase("menu") || cmd.equalsIgnoreCase("stop")) {
      menu = MENU_MAIN;
      showMainMenu();
      return;
    }

    if (cmd.startsWith("HUMISET")) {
      String valueStr = cmd.substring(7);
      valueStr.trim();
      long newInterval = valueStr.toInt();

      if (newInterval > 0) {
        humiInterval = newInterval;
        Serial.print("Nouvel intervalle Humi = ");
        Serial.print(humiInterval);
        Serial.println(" ms");
      } else {
        Serial.println("Erreur: donnée non valide.");
      }
      return;
    }

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
      setRelai(true);
      showRelaiMenu();
      return;
    }

    if (cmd.equalsIgnoreCase("OFF")) {
      setRelai(false);
      showRelaiMenu();
      return;
    }

    Serial.println("Commande inconnue.");
    showRelaiMenu();
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
