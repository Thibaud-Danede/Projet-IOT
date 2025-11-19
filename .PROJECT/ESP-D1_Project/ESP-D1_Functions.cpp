#include "ESP-D1_Functions.h"

/* =======================================================================
   CREATION DE LA PAGE WEB
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
  button {
    padding: 12px 25px;
    margin: 15px;
    font-size: 20px;
    border-radius: 8px;
    border: none;
    color: white;
    cursor: pointer;
  }
  .start { background: #4477dd; }
  .stop  { background: #cc3333; }
  #errorMsg { color: red; font-weight: bold; margin-top: 8px; }
</style>
</head>
<body>

<h1>Mesures du capteur DHT11</h1>

<div class="box">
  <p>Humidité : <span id="hum">--</span> %</p>
  <p>Température : <span id="temp">--</span> °C</p>
  <p>Heat Index : <span id="hi">--</span></p>
</div>

<h2>Contrôle du capteur</h2>

<button id="dhtBtn" class="start" onclick="toggleDHT()">Démarrer DHT</button>

<h3>Intervalle d’acquisition (ms)</h3>

<input type="number" id="intervalInput" min="100" step="1" style="font-size:20px; padding:5px;">
<button onclick="setIntervalValue()" style="background:#555;">Valider</button>

<div id="errorMsg"></div>

<script>

function syncState() {
  fetch("/dhtState")
    .then(r => r.json())
    .then(data => updateButton(data.active));
}

function updateButton(active) {
  const btn = document.getElementById("dhtBtn");

  if (active) {
    btn.textContent = "Arrêter DHT";
    btn.classList.replace("start", "stop");
  } else {
    btn.textContent = "Démarrer DHT";
    btn.classList.replace("stop", "start");

    document.getElementById("hum").textContent  = "--";
    document.getElementById("temp").textContent = "--";
    document.getElementById("hi").textContent   = "--";
  }
}

function syncInterval() {
  fetch("/dhtInterval")
    .then(r => r.json())
    .then(data => document.getElementById("intervalInput").value = data.interval);
}

function setIntervalValue() {
  const value = document.getElementById("intervalInput").value;
  const err = document.getElementById("errorMsg");

  if (value === "" || value < 100) {
    err.textContent = "L’intervalle doit être ≥ 100 ms.";
    return;
  }

  err.textContent = "";
  fetch("/setDHTinterval?value=" + value);
}

function toggleDHT() {
  const btn = document.getElementById("dhtBtn");
  const start = btn.classList.contains("start");

  fetch(start ? "/startDHT" : "/stopDHT")
    .then(() => updateButton(start));
}

function updateDHT() {
  const running = !document.getElementById("dhtBtn").classList.contains("start");
  if (!running) return;

  fetch("/dht")
    .then(r => r.json())
    .then(data => {
      document.getElementById("hum").textContent  = data.humidity.toFixed(1);
      document.getElementById("temp").textContent = data.temperature.toFixed(1);
      document.getElementById("hi").textContent   = data.heatindex.toFixed(1);
    });
}

syncState();
syncInterval();
updateDHT();
setInterval(updateDHT, 100);

</script>

</body>
</html>
)rawliteral";

    return html;
}

/* =======================================================================
   ACQUISITION DONNEES
   ======================================================================= */
String getDHTjson(float humidity, float temperature, float heatIndex) {
    return "{"
           "\"humidity\":" + String(humidity, 1) + "," +
           "\"temperature\":" + String(temperature, 1) + "," +
           "\"heatindex\":" + String(heatIndex, 1) +
           "}";
}


/* =======================================================================
   MENU AFFICHAGE
   ======================================================================= */
void showMainMenu() {
  Serial.println("\n======== MENU PRINCIPAL ========");
  Serial.println("Tapez dans l'invite de commande le numéro du paramètre que vous voulez régler");
  Serial.println("1 : Visualiser et gérer l’acquisition du DHT");
  Serial.println("2 : Visualiser et gérer l’accès WiFi");

}

void showDHTMenu() {
  Serial.println("\n=== MENU DHT ===");
  Serial.println("Affichage des valeurs du dht...");
  Serial.println("Commandes disponibles :");
  Serial.println("  DHTSET <ms>     : changer intervalle lecture");
  Serial.println("  RETURN ou MENU  : retour au menu principal\n");
}

void showWifiMenu() {
  Serial.println("\n=== MENU WIFI ===");
  Serial.print("Mode actuel : ");
  Serial.println(wifiMode == WIFI_MODE_AP ? "AP" : "WS");
  Serial.print("Auto-reconnexion WS : ");
  Serial.println(autoReconnectEnabled ? "ON" : "OFF");


  Serial.println("Commandes disponibles :");
  Serial.println("       AP        : passer en Access Point");
  Serial.println("       WS        : passer en Station");
  Serial.println("       RECO      : Activer/Désactiver reconnection automatique");
  Serial.println("       KILL      : passer en mode KO");
  Serial.println("  RETURN ou MENU : retour au menu principal\n");
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

    // MQTT publication 
    String payload = "{\"humidity\":" + String(humidity,1) + ",\"temperature\":" + String(temperature,1) + ",\"heatIndex\":" + String(heatIndex,1) + ",\"signature\":\"Thibaud\"}";
    mqttPublish(payload);
  }
}
