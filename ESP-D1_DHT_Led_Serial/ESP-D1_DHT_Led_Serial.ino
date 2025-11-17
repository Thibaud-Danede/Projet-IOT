/*********************************
* Simple led blinking 
* With Serial link at 115200 bauds
* 
* DHT sensor on D4
* 
* Board : ESP-D1  
* Author : O. Patrouix ESTIA
* Date : 29/09/2021
*********************************/

// constants won't change. Used here to 
// set pin numbers:
#include "ESP-D1_GPIO.h"

// DHT library and define
#include "DHT.h"
// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11

// Variables will change:
int StateD4 = LOW;            // ledState used to set the LED
long previousMillis_D4 = 0;   // will store last time LED was updated

// the folHIGH variables is a long because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long intervalMillis = 1000;           // interval at which to blink (milliseconds)

// Serial link data
String inputString = "";        // a string to hold incoming data
boolean stringComplete = false; // whether the string is complete

// DHT Object
float h;
float t;
float hic;

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(D4, DHTTYPE);

void setup() {
  // initialize digital esp8266 D4 as an output.
  pinMode(D4, OUTPUT);
  // Set D4 to StateD4
  digitalWrite(D4, StateD4);

  // Start DHT
  dht.begin();

  // Set RS232 to 115200 Bauds
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nBOOT ESP-D1 ready to work");
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
  // here is where you'd put code that needs to be running all the time.
  
  // Blink Led still alive
  // BlinkLed(D4, &StateD4, &previousMillis_D4, intervalMillis/2);

  // DHT readings
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  t = dht.readTemperature();
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
  }
  else {
    // Compute heat index in Celsius (isFahreheit = false)
    hic = dht.computeHeatIndex(t, h, false);
  
    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.print(t);
    Serial.print(F("°C "));
    Serial.print(F("  Heat index: "));
    Serial.println(hic);
  }


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
