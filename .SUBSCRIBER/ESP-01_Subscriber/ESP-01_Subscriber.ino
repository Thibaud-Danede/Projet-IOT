/********************
 *  This sketch demonstrates how to set up:
 *    - a WiFi access point
 *    - provide a web server on it.
 *    
 *  The server will set a GPIO pin depending on the request
 *    http://server_ip/Temp will measure temperature,
 *  server_ip is the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected.
 *  
 *  Connect to wifi hub and provide a web server on it.
 *  Author : O. Patrouix / ESTIA
 *  
 *  boards:
 *    - ESP01
 *    - Temperature motherboard DS18B20 
 *  Sensor on GPIO_2
 *  Blink Led on GPIO_0
******************/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

/* Set these to your desired credentials. */
const char *ssid = "!!**IIoT4**!!";
const char *password = "12345678";
IPAddress local_IP(192,168,4,29);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);
IPAddress myIP;

// constants won't change. Used here to 
// set pin numbers:
const int GPIO_0 =  0;      // the number of the LED pin
const int GPIO_2 =  2;      // the number of the LED pin
const int PIN_RELAIS = D1;  // pin relai selon câblage actuel

ESP8266WebServer server(80);

// Variables will change:
int StateGPIO_0 = HIGH;           // ledState used to set the LED
int StateGPIO_2 = LOW;            // ledState used to set the LED
long previousMillis_2 = 0;        // will store last time LED was updated
bool etatRelais = false; //Variable bool pour ON/OFF relai

// the folHIGH variables is a long because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long intervalMillis = 1000;           // interval at which to blink (milliseconds)

/* Just a little web site in a web browser
   connected to this access point to see it. 
*/
// Setup HTML page
String getPage(){
  // Confert IP to String
  String myIP_S = String(myIP[0]) + "." + String(myIP[1]) + "." + String(myIP[2]) + "." + String(myIP[3]);
  
   // Texte & classe selon l'état du relais
  String etatText  = etatRelais ? "ON" : "OFF";
  String etatClass = etatRelais ? "on" : "off";

  // Page HTML
  String page = "";
  page += "<!DOCTYPE html><html lang='fr'><head>";
  page += "<meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  page += "<title>Relay IoT</title>";

  // CSS
  page += "<style>";
  page += "body{margin:0;font-family:system-ui,-apple-system,Segoe UI,Roboto,sans-serif;";
  page += "background:linear-gradient(135deg,#0f172a,#1e293b);color:#e5e7eb;";
  page += "display:flex;align-items:center;justify-content:center;min-height:100vh;}";
  page += ".card{background:rgba(15,23,42,0.95);border-radius:18px;padding:32px 28px;";
  page += "box-shadow:0 18px 45px rgba(15,23,42,0.8);max-width:420px;width:90%;text-align:center;}";
  page += "h1{margin:0 0 4px 0;font-size:28px;}";
  page += ".subtitle{font-size:14px;color:#9ca3af;margin-bottom:24px;}";
  page += ".ip{font-size:20px;font-weight:600;margin-bottom:16px;}";
  page += ".status-label{font-size:14px;color:#9ca3af;margin-bottom:8px;}";
  page += ".badge{display:inline-block;padding:6px 16px;border-radius:999px;";
  page += "font-weight:600;font-size:14px;margin-bottom:24px;}";
  page += ".badge-on{background:rgba(22,163,74,0.18);color:#bbf7d0;";
  page += "border:1px solid rgba(34,197,94,0.7);}";
  page += ".badge-off{background:rgba(220,38,38,0.16);color:#fecaca;";
  page += "border:1px solid rgba(248,113,113,0.7);}";
  page += ".btn{display:inline-block;margin-top:8px;padding:16px 40px;border-radius:999px;";
  page += "border:none;text-decoration:none;font-size:18px;font-weight:600;";
  page += "cursor:pointer;transition:transform .12s ease,box-shadow .12s ease,background .12s ease;";
  page += "color:#0b1120;background:linear-gradient(135deg,#34d399,#22c55e);";
  page += "box-shadow:0 10px 25px rgba(34,197,94,0.5);}";
  page += ".btn:hover{transform:translateY(-1px);box-shadow:0 16px 35px rgba(34,197,94,0.65);}";
  page += ".btn:active{transform:translateY(1px);box-shadow:0 6px 18px rgba(34,197,94,0.45);}";
  page += ".footer{margin-top:18px;font-size:12px;color:#6b7280;}";
  page += "</style>";

  page += "</head><body>";
  page += "<div class='card'>";
  page += "<h1>Relay IoT</h1>";
  page += "<div class='subtitle'>Point d'accès embarqué</div>";

  page += "<div class='ip'>IP : " + myIP_S + "</div>";

  page += "<div class='status-label'>État du relais</div>";
  page += "<div class='badge badge-" + etatClass + "'>" + etatText + "</div>";

  page += "<div><a class='btn' href='http://" + myIP_S + "/open'>Basculer le relais</a></div>";
  
  page += "</div></body></html>";

  return page;
  
}

// To Do on http://192.168.4.XX/
// Set GPIO_0 to HIGH <=> relay opened
void handleRoot() {
  //server.send(200, "text/html", "<h1>You are connected</h1>");
  server.send(200, "text/html", getPage());
  digitalWrite(GPIO_0, HIGH);
}
// To Do on http://192.168.4.XX/open
// Set GPIO_0 to LOW for 500ms <=> relay closed
void handleOpen() {
  //server.send(200, "text/html", "<h1>Gate Opening</h1>");
  server.send(200, "text/html", getPage());

  // ON/OFF relai
  etatRelais = !etatRelais;
  digitalWrite(PIN_RELAIS, etatRelais ? HIGH : LOW);

  // set the LED with the ledState of the variable:
  StateGPIO_0 = LOW;
  digitalWrite(GPIO_0, StateGPIO_0);
  delay(500);
  StateGPIO_0 = HIGH;
  digitalWrite(GPIO_0, StateGPIO_0);
}

// To Do on http://192.168.4.XX/close
// Set GPIO_0 to LOW for 500ms <=> relay closed
void handleClose() {
  //server.send(200, "text/html", "<h1>Gate Closing</h1>");
  server.send(200, "text/html", getPage());
  // set the LED with the ledState of the variable:
  StateGPIO_0 = LOW;
  digitalWrite(GPIO_0, StateGPIO_0);
  delay(500);
  StateGPIO_0 = HIGH;
  digitalWrite(GPIO_0, StateGPIO_0);
}
  
void setup() {
  // initialize digital esp8266 gpio 0 as an output.
  pinMode(GPIO_0, OUTPUT);
  digitalWrite(GPIO_0, HIGH);
  // initialize digital esp8266 gpio 2 as an output.
  pinMode(GPIO_2, OUTPUT);
  digitalWrite(GPIO_2, LOW);

  // --> Ajoute :
  pinMode(PIN_RELAIS, OUTPUT);
  digitalWrite(PIN_RELAIS, LOW);   // relais au repos (à adapter selon ton module)
  // ---

  etatRelais = false;

  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");
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
}

// Blink Led on GPIO_X
// 50% duty cycle intervalMillis is 1/2 period
// Parameters : Pin | pointer to StateGPIO | pointer to PreviousMillis
void BlinkLed(int GPIO, int *StateGPIO, long *previousMillis, long intervalMillis){
  // check to see if it's time to blink the LED; that is, if the 
  // difference between the current time and last time you blinked 
  // the LED is bigger than the interval at which you want to 
  // blink the LED.
  unsigned long currentMillis = millis();
 
  if(currentMillis - *previousMillis > intervalMillis) {
    // save the last time you blinked the LED 
    *previousMillis = currentMillis;   

    // if the LED is off turn it on and vice-versa:
    if (*StateGPIO == HIGH)
    {
      *StateGPIO = LOW;
    }
    else
    {
      *StateGPIO = HIGH;
    }
    // set the LED with the ledState of the variable:
    digitalWrite(GPIO, *StateGPIO);
  }
}
  
void loop() {
  // Blink Led still alive
  BlinkLed(GPIO_2, &StateGPIO_2, &previousMillis_2, intervalMillis);
  // Handle server
  server.handleClient();
}
