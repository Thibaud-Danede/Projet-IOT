
// constants won't change. Used here to 
// set pin numbers:
#include "ESP-D1_GPIO.h"
// Onboard LED on D4 pin

#include <ESP8266WiFi.h> // Enables the ESP8266 to connect to the local network (via WiFi)
#include <PubSubClient.h> // Allows us to connect to, and publish to the MQTT broker

// WiFi
// Make sure to update this for your own WiFi network!
const char* ssid = "IIoT_A";
const char* password = "IIoT_AS3";
IPAddress local_IP(192,168,1,30);
IPAddress gateway(192,168,1,12);
IPAddress subnet(255,255,255,0);
IPAddress myIP;

// MQTT
// Make sure to update this for your own MQTT Broker!
const char* mqtt_server = "192.168.1.2";
const char* mqtt_topic = "TestSerial";
const char* mqtt_username = "usrThi";
const char* mqtt_password = "usrThi";
// The client id identifies the ESP8266 device. Think of it a bit like a hostname (Or just a name, like Greg).
const char* clientID = "ESP8266_Pub0";

// Variables will change:
int StateD4 = LOW;          // ledState used to set the LED
long previousMillis_D4 = 0;        // will store last time LED was updated

// the folHIGH variables is a long because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long intervalMillis = 1000;           // interval at which to blink (milliseconds)

// Serial link data
String inputString = "";        // a string to hold incoming data
boolean stringComplete = false; // whether the string is complete
boolean cmd = false; // whether the MQTT Message

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
PubSubClient client(mqtt_server, 1883, wifiClient); // 1883 is the listener port for the Broker

void setup() {
  // initialize digital esp8266 D4 as an output.
  pinMode(D4, OUTPUT);
  // Set D4 to StateD4
  digitalWrite(D4, StateD4);

  // Set RS232 to 115200 Bauds
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nBOOT ESP-D1 ready to work");

    // Connecting
  Serial.print("Connecting to ");
  Serial.println(ssid);

 // Start WIFI
  WiFi.mode(WIFI_STA);
  // Change the IP address 
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);
  // Wait while connecting
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conected");

  // Print the IP address
  myIP = WiFi.localIP();
  Serial.print("Local IP address: ");
  Serial.println(myIP);

  // Connect to MQTT Broker
  // client.connect returns a boolean value to let us know if the connection was successful.
  // If the connection is failing, make sure you are using the correct MQTT Username and Password (Setup Earlier in the Instructable)
  if (client.connect(clientID, mqtt_username, mqtt_password)) {
    Serial.println("Connected to MQTT Broker!");
    Serial.println("As user : " + String(mqtt_username));
    Serial.println("Topic: " + String(mqtt_topic));
  }
  else {
    Serial.println("Connection to MQTT Broker failed...");
  }
}

void loop() {
  // here is where you'd put code that needs to be running all the time.
  
  // Blink Led still alive
  //BlinkLed(D4, &StateD4, &previousMillis_D4, intervalMillis/2);

  // Check if Data available on Serial and merge them into inputString
  serialEvent();
  
  // print the string when a newline arrives:
  if (stringComplete) {
    Serial.print(inputString); 
    if(inputString.charAt(0) == 'M')
      cmd = true;
    else
      Serial.println("cmd failed");
    // clear the string:
    inputString = "";
    stringComplete = false;
  }

  // MQTT publish if cmd is true
    if (cmd) {
    // PUBLISH to the MQTT Broker (topic = mqtt_topic, defined at the beginning)
    // Here, "Command M received" is the Payload, but this could be changed to a sensor reading, for example.
    if (client.publish(mqtt_topic, "Command M received")) {
      Serial.println("Command M received and message sent!");
    }
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
    else {
      Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
      client.connect(clientID, mqtt_username, mqtt_password);
      delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
      client.publish(mqtt_topic, "Command M received");
    }
    // Clear Flag for next command
    cmd = false;
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
