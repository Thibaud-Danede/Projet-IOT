/*********************************
* Simple led blinking 
* With Serial link at 115200 bauds
* Blink Onboard Led on D4
* Push Button on D7 (PullUp) => scrutation
* Board : ESP-D1  
* Author : O. Patrouix ESTIA
* Date : 29/09/2021
*********************************/
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
   SETUP WEBPAGE
   ======================================================================= */
String getPage() {

  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<title>DHT Monitor</title>
<style>
  body { font-family: Arial; text-align: center; background: #eef; padding-top: 40px; }
  .box { display: inline-block; padding: 20px; background: white; border-radius: 10px; box-shadow: 0 0 10px #aaa; }
  h1 { color: #006; }
  .value { font-size: 2em; color: #333; }
</style>
</head>
<body>

<h1>Mesures du capteur DHT11</h1>
<div class="box">
  <p>Humidité : <span id="hum">--</span> %</p>
  <p>Température : <span id="temp">--</span> °C</p>
  <p>Heat Index : <span id="hi">--</span></p>
</div>

<script>
function updateDHT() {
  fetch("/dht")
    .then(response => response.json())
    .then(data => {
      document.getElementById("hum").textContent = data.humidity.toFixed(1);
      document.getElementById("temp").textContent = data.temperature.toFixed(1);
      document.getElementById("hi").textContent = data.heatindex.toFixed(1);
    })
    .catch(err => console.log("Erreur:", err));
}

setInterval(updateDHT, 1000); // mise à jour toutes les secondes
updateDHT(); // mise à jour immédiate au chargement
</script>

</body>
</html>
)rawliteral";

  return html;
}


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

  // Start DHT
  dht.begin();

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  delay(300);
  Serial.println("\nBOOT ESP-D1 ready to work");

  showMainMenu();

  // initialize digital esp8266 gpio 0 as an output. MODIFIER GPIO
  pinMode(WEB0, OUTPUT);
  digitalWrite(WEB0, HIGH);
  // initialize digital esp8266 gpio 2 as an output. MODIFIER GPIO
  pinMode(WEB2, OUTPUT);
  digitalWrite(WEB2, LOW);


  //SETUP WEB
  // Setup the WIFI access point 
  WiFi.mode(WIFI_AP);
  // Change the IP address 
  WiFi.softAPConfig(local_IP, gateway, subnet);
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);
  // Get IP to see if IP is not default 192.168.4.1 
  myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/open", handleOpen);
  server.on("/close", handleClose);
  server.begin();
  Serial.println("HTTP server started");

  //Handler de la page WEB
  server.on("/", []() {
  server.send(200, "text/html", getPage());
  });
  server.on("/dht", []() {
  String json = "{";
  json += "\"humidity\":" + String(humidity, 1) + ",";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"heatindex\":" + String(heatIndex, 1);
  json += "}";

  server.send(200, "application/json", json);
  });


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


