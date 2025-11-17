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
const char *ssid = "!!AA!!";
const char *password = "12345678";
IPAddress local_IP(192,168,4,99);
IPAddress gateway(192,168,4,99);
IPAddress subnet(255,255,255,0);
IPAddress myIP;

// constants won't change. Used here to 
// set pin numbers:
const int GPIO_0 =  0;      // the number of the LED pin
const int GPIO_2 =  2;      // the number of the LED pin

ESP8266WebServer server(80);

// Variables will change:
int StateGPIO_0 = HIGH;           // ledState used to set the LED
int StateGPIO_2 = LOW;            // ledState used to set the LED
long previousMillis_2 = 0;        // will store last time LED was updated

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
  // HTML Page
  String page = "<html lang=fr-FR><head><meta http-equiv='Content-Type'/>";
  page += "<title>Relay IoT</title>";
  page += "<style> body { background-color: #fffff; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }";
  page+= ".btn { border: none; ";
  page += "background-color: green; ";
  page += "padding: 50px 150px; ";
  page += "font-size: 40px; ";
  page += "cursor: pointer; ";
  page += "display: inline-block; } ";
  page += ".success { color: white; } ";
  page += "</style>";
  page += "</head><body>";
  page += "<p style='text-align: center'>&nbsp;</p> ";
  page += "<p style='text-align: center'><font size=\"+6\">";
  page += myIP_S;
  page += "</font></p>";
  page += "<p style='text-align: center'>&nbsp;</p>";
  page += "<p style='text-align: center'>&nbsp;</p>";
  page += "<p style='text-align: center'>&nbsp;</p>";
  page += "<p style='text-align: center'><a class='btn success' href='http://" + myIP_S + "/open' role='button'>Refresh</a></p>";
  page += "</body></html>";
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
