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
<title>DHT + WiFi Dashboard</title>

<style>
  body {
    font-family: Arial;
    background: #eef;
    padding: 25px;
  }

  .container {
    display: flex;
    justify-content: center;
    align-items: flex-start;
    gap: 50px;
    margin-top: 20px;
    flex-wrap: wrap;
  }

  .column.dhtBox {
    background: #ffffff;
    padding: 30px;
    border-radius: 15px;
    width: 420px;
    box-shadow: 0 0 18px rgba(0,0,0,0.25);
    border-left: 8px solid #3366ff;
    text-align: center;
  }

  .column.wifiBox {
    background: #ffffff;
    padding: 25px;
    border-radius: 12px;
    width: 320px;
    box-shadow: 0 0 12px rgba(0,0,0,0.20);
    text-align: center;
  }

  h1 {
    text-align: center;
    color: #003;
    margin-bottom: 10px;
  }

  h2 {
    color: #003;
    margin-bottom: 18px;
  }

  h3 {
    margin-top: 20px;
    color: #004;
  }

  .value {
    font-size: 32px;
    font-weight: bold;
    color: #003399;
  }

  button {
    padding: 12px 25px;
    margin-top: 12px;
    font-size: 18px;
    border-radius: 8px;
    border: none;
    color: white;
    cursor: pointer;
    display: block;
    margin-left: auto;
    margin-right: auto;
  }

  .start { background: #4477dd; }
  .stop  { background: #cc3333; }

  .wifiBtn { background:#228855; width: 80%; }
  .wifiKO  { background:#aa2222; width: 80%; }

  #errorMsg { color: red; font-weight: bold; margin-top: 10px; }
</style>
</head>
<body>

<h1>Dashboard Capteur + WiFi</h1>

<div class="container">

  <!-- ===== COLONNE DHT ===== -->
  <div class="column dhtBox">

    <h2>Mesures DHT11</h2>

    <p>Humidité : <span class="value" id="hum">--</span> %</p>
    <p>Température : <span class="value" id="temp">--</span> °C</p>
    <p>Heat Index : <span class="value" id="hi">--</span></p>

    <h3>Contrôle du capteur</h3>
    <button id="dhtBtn" class="start" onclick="toggleDHT()">Démarrer DHT</button>

    <h3>Intervalle d’acquisition (ms)</h3>
    <input type="number" id="intervalInput" min="100" step="1"
           style="font-size:18px; padding:8px; width:160px; text-align:center;">
    <br>
    <button onclick="setIntervalValue()" style="background:#555; margin-top:10px;">Valider</button>

    <div id="errorMsg"></div>
  </div>



  <!-- ===== COLONNE WIFI ===== -->
  <div class="column wifiBox">

    <h2>État WiFi</h2>

    <p>Mode actuel : <span class="value" id="wm">--</span></p>
    <p>Connexion : <span class="value" id="wc">--</span></p>
    <p>Adresse IP : <span class="value" id="wip">--</span></p>
    <p>Reconnexion auto : <span class="value" id="wr">--</span></p>

    <br>

    <button onclick="setAP()" class="wifiBtn">Passer en AP</button>
    <button onclick="setWS()" class="wifiBtn">Passer en WS</button>
    <button onclick="setKO()" class="wifiKO">Passer en KO</button>
    <button onclick="toggleReco()" style="background:#555; width:80%; margin-top:15px;">
      Toggle Reconnexion Auto
    </button>
  </div>

</div>


<script>

///////////////////////////////////
//        DHT FUNCTIONS
///////////////////////////////////

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



///////////////////////////////////
//        WIFI MANAGEMENT
///////////////////////////////////

function refreshWifi() {
  fetch("/wifiState")
    .then(r => r.json())
    .then(data => {
      document.getElementById("wm").textContent  = data.mode;
      document.getElementById("wc").textContent  = data.connected ? "Connecté" : "Déconnecté";
      document.getElementById("wip").textContent = data.ip;
      document.getElementById("wr").textContent  = data.autoreco ? "ON" : "OFF";
    });
}


// === POPUP DE CONFIRMATION POUR CHAQUE ACTION ===

function setAP() {
  if (confirm("⚠️ Vous allez passer en mode AP.\nVous devrez vous reconnecter au WiFi de l’ESP.\nContinuer ?")) {
    fetch("/wifiAP").then(refreshWifi);
  }
}

function setWS() {
  if (confirm("⚠️ Passage en mode WS.\nSi la connexion échoue, l’accès au site sera perdu.\nContinuer ?")) {
    fetch("/wifiWS").then(refreshWifi);
  }
}

function setKO() {
  if (confirm("⚠️ MODE KO : Le WiFi sera désactivé.\nVous perdrez immédiatement l’accès au site.\nContinuer ?")) {
    fetch("/wifiKO").then(refreshWifi);
  }
}

function toggleReco() {
  if (confirm("Basculer l'état de la reconnexion automatique ?")) {
    fetch("/wifiRecoToggle").then(refreshWifi);
  }
}



///////////////////////////////////
//            INIT
///////////////////////////////////

syncState();
syncInterval();
updateDHT();
setInterval(updateDHT, 120);

refreshWifi();
setInterval(refreshWifi, 2000);

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
  Serial.print("wifiMode = ");
  switch (wifiMode) {
    case WIFI_MODE_AP: Serial.println("AP"); break;
    case WIFI_MODE_WS: Serial.println("WS"); break;
    case WIFI_MODE_KO: Serial.println("KO"); break;
  }

  // --- État réel du WiFi ---
  Serial.print("Etat WiFi réel : ");
  int st = WiFi.status();   // ESP8266 : retourne un int
  if (st == WL_CONNECTED) Serial.println("CONNECTÉ");
  else Serial.println("DÉCONNECTÉ");

  Serial.print("Auto-reconnexion WS : ");
  Serial.println(autoReconnectEnabled ? "ON" : "OFF");

  Serial.println("\nCommandes disponibles :");
  Serial.println("  AP       : passer en Access Point");
  Serial.println("  WS       : passer en Station");
  Serial.println("  RECO     : activer/désactiver reconnect auto");
  Serial.println("  KILL     : mettre le WiFi en KO");
  Serial.println("  RETURN   : retour menu principal\n");
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
