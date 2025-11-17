/*********************************
* Simple led blinking 
* With Serial link at 115200 bauds
* Blink Onboard Led on D4
* 
* This sketch demonstrates how to set up:
*    - a WiFi Station AND Access Point
*    - provide a web server on it.
*    
*  STATION Mode.  
*  WiFi Hub :
*    - ssid = "robotique";
*    - password = "robotiqueS3";
*  The IP address of the ESP8266 module, will be 
*  printed to Serial when the module is connected.  
*  server_ip is 192.168.1.XXX
*  http://192.168.1.XXX/gettemp will send temperature page.
*  
*  ACCESS POINT Mode
*  The IP address of the ESP8266 module, will be 
*  printed to Serial when the module is connected. 
*  server_ip is 192.168.4.XXX
*  http://192.168.4.XXX/gettemp will send temperature page.
*
* Board : ESP-D1  
* Author : O. Patrouix ESTIA
* Date : 29/09/2021
*********************************/

// constants won't change. Used here to 
// set pin numbers:
#include "ESP-D1_GPIO.h"
// Onboard LED on D4 pin
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

// STATION
//Set these to your desired credentials.
const char* WS_ssid = "robotique";
const char* WS_password = "robotiqueS3";
IPAddress WS_local_IP(192,168,1,29);
IPAddress WS_gateway(192,168,1,12);
IPAddress WS_subnet(255,255,255,0);
IPAddress WS_myIP;
// ACCESS POINT
//Set these to your desired credentials.
const char *AP_ssid = "IIoT_Temp";
const char *AP_password = "";
IPAddress AP_local_IP(192,168,4,22);
IPAddress AP_gateway(192,168,4,9);
IPAddress AP_subnet(255,255,255,0);
IPAddress AP_myIP;
// Create an instance of the server
// specify the port to listen on as an argument
ESP8266WebServer server(80);

// Variables will change:
int StateD4 = LOW;          // ledState used to set the LED
long previousMillis_D4 = 0;        // will store last time LED was updated

// the folHIGH variables is a long because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long intervalMillis = 1000;           // interval at which to blink (milliseconds)

// Serial link data
String inputString = "";        // a string to hold incoming data
boolean stringComplete = false; // whether the string is complete

void setup() {
  // initialize digital esp8266 D4 as an output.
  pinMode(D4, OUTPUT);
  // Set D4 to StateD4
  digitalWrite(D4, StateD4);

  // Set RS232 to 115200 Bauds
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nBOOT ESP-D1 ready to work");

  // Connect to WiFi network
  Serial.println();
  Serial.println("Configuring Wifi...");
  // Start WIFI
  // Setup the WIFI AP ou WTA or Both AP+STA
  // WiFi.mode(WIFI_AP);
  // WiFi.mode(WIFI_STA);
  WiFi.mode(WIFI_AP_STA);
  // STATION Change the IP address 
  WiFi.config(WS_local_IP, WS_gateway, WS_subnet);
  WiFi.begin(WS_ssid, WS_password);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WS_ssid);
  // Wait while connecting
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  // Print the IP address
  WS_myIP = WiFi.localIP();
  Serial.print("Local IP address: ");
  Serial.println(WS_myIP);
  Serial.println("");

  Serial.println("Configuring access point...");
  // Change the IP address 
  WiFi.softAPConfig(AP_local_IP, AP_gateway, AP_subnet);
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(AP_ssid, AP_password);
  // Get IP to see if IP is not default 192.168.4.1 
  AP_myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(AP_myIP);

  Serial.println("WiFi connected");
  // Start the server
  server.begin();
  Serial.println("Web server On");

  // Link functions to URL
  server.on("/", handleRoot);
  server.on("/gettemp", handleGetTemp);
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

/* Just a little web site in a web browser
   connected to this access point to see it. 
*/
// Setup HTML page
String getPage(){
  // Confert IP to String
  String WS_myIP_S = String(WS_myIP[0]) + "." + String(WS_myIP[1]) + "." + String(WS_myIP[2]) + "." + String(WS_myIP[3]);
  String AP_myIP_S = String(AP_myIP[0]) + "." + String(AP_myIP[1]) + "." + String(AP_myIP[2]) + "." + String(AP_myIP[3]);
  // HTML Page
  String page = "<html lang=fr-FR><head><meta http-equiv='Content-Type'/>";
  page += "<title>Temperature IoT</title>";
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
  page += "Station IP" + WS_myIP_S;
  page += "</font></p>";
  page += "<p style='text-align: center'><font size=\"+6\">";
  page += "Access Point IP" + AP_myIP_S;
  page += "</font></p>";
  page += "<p style='text-align: center'>&nbsp;</p>";

  //String T = String(Temperature,2);
  String T = "0.00";
  page += "<p style='text-align: center'><font size=\"+6\">Temperature is ";
  page += T;
  page += " &#186;C</font></p>";
  
  page += "<p style='text-align: center'>&nbsp;</p>";
  page += "<p style='text-align: center'>&nbsp;</p>";
  page += "<p style='text-align: center'><a class='btn success' href='http://" + WS_myIP_S + "/gettemp' role='button'>Get Temp WS</a></p>";
  page += "<p style='text-align: center'>&nbsp;</p>";
  page += "<p style='text-align: center'><a class='btn success' href='http://" + AP_myIP_S + "/gettemp' role='button'>Get Temp AP</a></p>";
  page += "</body></html>";
  return page;
}

// To Do on http://192.168.X.XX/
void handleRoot() {
  server.send(200, "text/html", getPage());
}
// To Do on http://192.168.X.XX/gettemp
void handleGetTemp() {
  server.send(200, "text/html", getPage());
}
void loop() {
  // here is where you'd put code that needs to be running all the time.
  // Handle server
  server.handleClient();
    
  // Blink Led still alive
  BlinkLed(D4, &StateD4, &previousMillis_D4, intervalMillis/2);

  // Check if Data available on Serial and merge them into inputString
  serialEvent();
  
  // print the string when a newline arrives:
  if (stringComplete) {
    Serial.print(inputString); 
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
    char inChar;
    while(Serial.available() > 0) {
      // get the new byte:
      char inChar = (char)Serial.read(); 
      // add it to the inputString:
      inputString += inChar;
      // if the incoming character is a newline, set a flag
      // so the main loop can do something about it:
      if (inChar == '\n') {
        stringComplete = true;
      }
    } 
}
