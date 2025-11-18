#ifndef ESP_D1_FUNCTIONS_H
#define ESP_D1_FUNCTIONS_H

#include <Arduino.h>

// Renvoie la page HTML complète
String getPage();

// Renvoie un JSON formaté pour les mesures DHT
String getDHTjson(float humidity, float temperature, float heatIndex);

#endif
