#ifndef ESP_D1_FUNCTIONS_H
#define ESP_D1_FUNCTIONS_H

#include <Arduino.h>

// Renvoie la page HTML complète
String getPage();

// Renvoie un JSON formaté pour les mesures DHT
String getDHTjson(float humidity, float temperature, float heatIndex);

// Fonction de menuing
void showMainMenu();
void showDHTMenu();
void showWifiMenu();

// Lecture des données
#include "DHT.h"
extern DHT dht;
extern float humidity;
extern float temperature;
extern float heatIndex;
extern bool autoReconnectEnabled;
void readDHT();


// Network
enum WifiModeState {
  WIFI_MODE_AP,
  WIFI_MODE_WS,
  WIFI_MODE_KO
};
extern WifiModeState wifiMode;

// MQTT Managment
void mqttPublish(String payload);


#endif
