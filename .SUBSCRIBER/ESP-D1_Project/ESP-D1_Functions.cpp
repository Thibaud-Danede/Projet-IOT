#include "ESP-D1_Functions.h"

String getPage() {
  // String myIP_S = String(myIP[0]) + "." + String(myIP[1]) + "." +
  //                 String(myIP[2]) + "." + String(myIP[3]);

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
</style>
</head>
<body>

<h1>Contrôle du relais</h1>

<div class="card">
  <div class="ip">Point d'accès : )rawliteral";

  //html += myIP_S;

  html += R"rawliteral(</div>

  <div class="state-label">État actuel :</div>
  <div id="relayBadge" class="badge off">--</div>

  <button id="relayBtn" onclick="toggleRelay()">Basculer le relais</button>

  <div id="errorMsg"></div>
</div>

<script>
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

// synchro initiale + maj régulière
syncState();
setInterval(syncState, 1000);
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
