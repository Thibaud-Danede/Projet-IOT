#include "ESP-D1_Functions.h"

String getPage() {

  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<title>Relay IoT</title>
<style>
  body {
    font-family: Arial, sans-serif;
    text-align: center;
    background: #eef;
    padding-top: 40px;
  }
  .card {
    display: inline-block;
    padding: 25px 35px;
    background: white;
    border-radius: 12px;
    box-shadow: 0 0 12px #aaa;
    min-width: 320px;
  }
  h1 { color: #003366; margin-top: 0; }
  .ip {
    font-size: 18px;
    margin-bottom: 10px;
    color: #444;
  }
  .state-label {
    font-size: 16px;
    margin-top: 10px;
  }
  .badge {
    display: inline-block;
    margin-top: 8px;
    padding: 6px 16px;
    border-radius: 999px;
    font-weight: bold;
    font-size: 16px;
  }
  .on  { background:#c6f6d5; color:#166534; border:1px solid #16a34a; }
  .off { background:#fecaca; color:#991b1b; border:1px solid #ef4444; }

  button {
    padding: 12px 28px;
    margin-top: 20px;
    font-size: 20px;
    border-radius: 999px;
    border: none;
    color: white;
    cursor: pointer;
    background: #2563eb;
    box-shadow: 0 4px 10px rgba(37,99,235,0.5);
  }
  button:hover {
    background:#1d4ed8;
  }

  #errorMsg { color:red; margin-top:10px; font-weight:bold; }

  .value {
    font-size: 22px;
    font-weight: bold;
    margin-top: 8px;
  }

  .alert {
    margin-top: 12px;
    padding: 10px 15px;
    border-radius: 8px;
    background:#fee2e2;
    color:#991b1b;
    border:1px solid #ef4444;
    font-weight:bold;
  }
</style>
</head>
<body>

<h1>Contrôle du relais & mesures DHT</h1>

<div class="card">


  <div class="state-label">État du relais :</div>
  <div id="relayBadge" class="badge off">--</div>

  <div class="state-label" style="margin-top:16px;">Humidité mesurée :</div>
  <div id="humVal" class="value">-- %</div>

  <div id="humAlert" class="alert" style="display:none;">
    Alerte : humidité &gt; 50&nbsp;% – relais activé
  </div>

  <div class="state-label" style="margin-top:16px;">Température mesurée :</div>
  <div id="tempVal" class="value">-- °C</div>

  <div id="tempAlert" class="alert" style="display:none;">
    Alerte : température &gt; 35&nbsp;°C – relais activé
  </div>

  <button id="relayBtn" onclick="toggleRelay()">Basculer le relais</button>

  <div id="errorMsg"></div>
</div>

<script>
// Met à jour l'affichage du relais (badge + texte du bouton)
function updateUI(state) {
  const badge = document.getElementById('relayBadge');
  const btn   = document.getElementById('relayBtn');

  if (state) {
    badge.textContent = "ON";
    badge.classList.remove('off');
    badge.classList.add('on');
    btn.textContent = "Éteindre le relais";
  } else {
    badge.textContent = "OFF";
    badge.classList.remove('on');
    badge.classList.add('off');
    btn.textContent = "Allumer le relais";
  }
}

// Récupère l'état du relais
function syncState() {
  fetch('/relayState')
    .then(r => r.json())
    .then(data => {
      updateUI(data.state);
      document.getElementById('errorMsg').textContent = "";
    })
    .catch(err => {
      document.getElementById('errorMsg').textContent =
        "Erreur de lecture de l'état du relais";
    });
}

// Récupère l'humidité + température (JSON généré par getDHTjson) et gère les alertes
function refreshDht() {
  fetch('/dht')
    .then(r => r.text())   // on lit en texte pour tolérer 'nan'
    .then(txt => {
      const humSpan   = document.getElementById('humVal');
      const tempSpan  = document.getElementById('tempVal');
      const humAlert  = document.getElementById('humAlert');
      const tempAlert = document.getElementById('tempAlert');

      // --- Humidité ---
      const humMatch = txt.match(/"humidity":([^,}]+)/);
      let hum = NaN;
      if (humMatch) hum = parseFloat(humMatch[1]);

      if (isNaN(hum)) {
        humSpan.textContent  = "-- %";
        humAlert.style.display = "none";
      } else {
        humSpan.textContent = hum.toFixed(1) + " %";
        humAlert.style.display = hum > 50.0 ? "block" : "none";
      }

      // --- Température ---
      const tempMatch = txt.match(/"temperature":([^,}]+)/);
      let temp = NaN;
      if (tempMatch) temp = parseFloat(tempMatch[1]);

      if (isNaN(temp)) {
        tempSpan.textContent  = "-- °C";
        tempAlert.style.display = "none";
      } else {
        tempSpan.textContent = temp.toFixed(1) + " °C";
        tempAlert.style.display = temp > 35.0 ? "block" : "none";
      }
    })
    .catch(err => {
      // en cas d'erreur on ne change rien à l'affichage
    });
}

// Appelé quand on clique sur le bouton HTML
function toggleRelay() {
  fetch('/toggleRelay')
    .then(r => r.json())
    .then(data => {
      updateUI(data.state);
      document.getElementById('errorMsg').textContent = "";
    })
    .catch(err => {
      document.getElementById('errorMsg').textContent =
        "Erreur lors de l'envoi de la commande";
    });
}

// Rafraîchissement combiné : état relais + DHT
function globalRefresh() {
  syncState();
  refreshDht();
}

// synchro initiale + maj régulière
globalRefresh();
setInterval(globalRefresh, 1000);
</script>

</body>
</html>
)rawliteral";

  return html;
}


String getDHTjson(float humidity, float temperature, float heatIndex) {
    return "{"
           "\"humidity\":" + String(humidity, 1) + "," +
           "\"temperature\":" + String(temperature, 1) + "," +
           "\"heatindex\":" + String(heatIndex, 1) +
           "}";
}
