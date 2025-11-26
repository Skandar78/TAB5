#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
void wsSend();   // prototype pour pouvoir l'utiliser avant sa définition

// =========================================================
//                    CONFIG WIFI
// =========================================================
const char* WIFI_SSID = "SKANDARPC7702";
const char* WIFI_PASS = "halouani";
// Identifiants d’accès au dashboard
const char* http_username = "admin";
const char* http_password = "admin";

// =========================================================
//                 SERVEURS & FIX IMPORTANT
// =========================================================
WebServer server(80);
WebSocketsServer ws(81);

// ---- FIX : déclaration anticipée de la fonction WebSocket ----
void wsEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len);
// ----------------------------------------------------------------

// =========================================================
//                    CAPTEUR IR
// =========================================================
const int IR_PIN = 1;

// =========================================================
//                      ETAT
// =========================================================
bool rawDetect = false;
bool stableDetect = false;
unsigned long lastChange = 0;
unsigned long sensitivity = 200;

unsigned long detectStart = 0;
bool alarmOn = false;
const unsigned long ALARM_MS = 2000;

unsigned long detectCount = 0;
unsigned long lastBeepMs = 0;   // pour le bip d'alarme

// =========================================================
//                    HISTORIQUE
// =========================================================
struct LogEvent { unsigned long t; bool val; };
LogEvent history[10];
int histPos = 0;
bool histFull = false;

// =========================================================
//               GRAPH TABLETTE (60 points)
// =========================================================
bool graphData[60];
int graphIndex = 0;
unsigned long lastGraphMs = 0;

// =========================================================
//                    LAYOUT TABLETTE
// =========================================================
int W, H;
int STATUS_X, STATUS_Y, STATUS_W, STATUS_H;
int GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H;
int HIST_X, HIST_Y;
int SLIDER_X, SLIDER_Y, SLIDER_W;
int BTN_RESET_X, BTN_RESET_Y, BTN_RESET_W, BTN_RESET_H;
int BTN_SOUND_X, BTN_SOUND_Y, BTN_SOUND_W, BTN_SOUND_H;

bool soundOn = true;

#define SENS_MIN 50
#define SENS_MAX 1000

// =========================================================
//               INIT LAYOUT (calcul dynamique)
// =========================================================
void initLayout() {
  W = M5.Lcd.width();
  H = M5.Lcd.height();

  STATUS_X = W * 0.05;
  STATUS_Y = H * 0.03;
  STATUS_W = W * 0.90;
  STATUS_H = H * 0.12;

  GRAPH_X = W * 0.05;
  GRAPH_Y = STATUS_Y + STATUS_H + 30;
  GRAPH_W = W * 0.90;
  GRAPH_H = H * 0.18;

  HIST_X = W * 0.05;
  HIST_Y = GRAPH_Y + GRAPH_H + 40;

  SLIDER_X = W * 0.05;
  SLIDER_Y = H * 0.72;
  SLIDER_W = W * 0.90;

  BTN_RESET_X = W * 0.05;
  BTN_RESET_Y = H * 0.82;
  BTN_RESET_W = W * 0.42;
  BTN_RESET_H = 60;

  BTN_SOUND_X = W * 0.53;
  BTN_SOUND_Y = H * 0.82;
  BTN_SOUND_W = W * 0.42;
  BTN_SOUND_H = 60;
}

// =========================================================
//                    DESSIN HISTORIQUE
// =========================================================
void drawHistory() {
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, BLACK);

  int lineH = 26;

  for (int i = 0; i < 10; i++) {
    if (!histFull && i >= histPos) break;

    int idx = (histFull ? (histPos + i) % 10 : i);

    unsigned long t = history[idx].t / 1000;
    int s = t % 60;
    int m = (t / 60) % 60;

    M5.Lcd.setCursor(HIST_X, HIST_Y + i * lineH);
    M5.Lcd.printf("%02d:%02d - %s", m, s,
                  history[idx].val ? "PROCHE" : "LOIN");
  }
}

// =========================================================
//                     GRAPH TEMPS RÉEL
// =========================================================
void addGraphPoint(bool v) {
  graphData[graphIndex] = v;
  graphIndex = (graphIndex + 1) % 60;
}

void drawGraph() {
  M5.Lcd.fillRect(GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H, BLACK);
  int baseY = GRAPH_Y + GRAPH_H - 10;

  M5.Lcd.drawRect(GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H, WHITE);

  int lx = 0, ly = 0;
  bool first = true;

  for (int i = 0; i < 60; i++) {
    int idx = (graphIndex + i) % 60;
    int x = GRAPH_X + (i * GRAPH_W) / 60;
    int y = graphData[idx] ? (GRAPH_Y + 10) : baseY;

    if (!first)
      M5.Lcd.drawLine(lx, ly, x, y,
                      graphData[idx] ? GREEN : RED);

    lx = x;
    ly = y;
    first = false;
  }
}

// =========================================================
//                         DASHBOARD
// =========================================================
void drawDashboard(const char* status, uint16_t col) {
  M5.Lcd.fillScreen(BLACK);

  M5.Lcd.fillRoundRect(STATUS_X, STATUS_Y, STATUS_W, STATUS_H, 15, col);
  M5.Lcd.setTextColor(BLACK, col);
  M5.Lcd.setTextSize(5);
  M5.Lcd.setCursor(STATUS_X + STATUS_W * 0.20,
                   STATUS_Y + STATUS_H * 0.25);
  M5.Lcd.print(status);

  drawGraph();
  drawHistory();

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(SLIDER_X, SLIDER_Y - 30);
  M5.Lcd.printf("Sensibilite : %lu ms", sensitivity);

  M5.Lcd.drawRect(SLIDER_X, SLIDER_Y, SLIDER_W, 8, WHITE);
  float ratio = (float)(sensitivity - SENS_MIN) / (float)(SENS_MAX - SENS_MIN);
  int cx = SLIDER_X + ratio * SLIDER_W;
  M5.Lcd.fillCircle(cx, SLIDER_Y + 4, 10, CYAN);

  M5.Lcd.fillRoundRect(BTN_RESET_X, BTN_RESET_Y,
                       BTN_RESET_W, BTN_RESET_H,
                       12, DARKGREY);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(BTN_RESET_X + 20, BTN_RESET_Y + 15);
  M5.Lcd.print("RESET");

  uint16_t sc = soundOn ? GREEN : RED;
  M5.Lcd.fillRoundRect(BTN_SOUND_X, BTN_SOUND_Y,
                       BTN_SOUND_W, BTN_SOUND_H,
                       12, sc);
  M5.Lcd.setTextColor(BLACK);
  M5.Lcd.setCursor(BTN_SOUND_X + 20, BTN_SOUND_Y + 15);
  M5.Lcd.print(soundOn ? "SON ON" : "SON OFF");
}

// =========================================================
//                   PAGE WEB HTML + JS
// =========================================================
String webPage() {
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset='utf-8'>
<title>Tab5 Dashboard</title>
<meta name='viewport' content='width=device-width,initial-scale=1.0'>
<style>
body { background:#111; color:#fff; font-family:Arial; text-align:center; margin:0; }
.card { background:#222; padding:20px; border-radius:12px; width:90%; margin:15px auto; }
canvas { background:#000; border:1px solid #444; border-radius:10px; }
button { padding:12px 20px; font-size:1.2em; border-radius:10px; border:none; cursor:pointer; }
#resetBtn { background:#555; color:white; }
#soundBtn { background:#2d5; color:white; }
</style>
</head>

<body>

<h2>Tablette Tab5 - Dashboard Web</h2>


<div class='card'>
  <h3>État</h3>
  <div id='status' style="font-size:2.5em; padding:20px; border-radius:12px; background:#333;">LOIN</div>
  <p>Détections : <span id='count'>0</span></p>
  <p>Alarme : <span id='alarm'>NON</span></p>
  <button id='resetBtn' onclick="fetch('/reset')">Reset</button>
  <button id='soundBtn' onclick="fetch('/sound')">Son OFF</button>
</div>

<div class='card'>
  <h3>Radar</h3>
  <canvas id='radar' width='260' height='260'></canvas>
</div>

<div class='card'>
  <h3>Bargraph (détections/minute)</h3>
  <canvas id='bar' width='300' height='200'></canvas>
</div>

<script>

let ws;
let lastValue = 0;
let radarAngle = 0;

let barData = new Array(10).fill(0);
let lastMinute = new Date().getMinutes();

function drawBar() {
  let c = document.getElementById('bar');
  let ctx = c.getContext('2d');
  ctx.clearRect(0, 0, c.width, c.height);

  let max = Math.max(...barData, 5);       // pour éviter division par 0
  let bw  = c.width / barData.length;

  ctx.font = "14px Arial";
  ctx.textAlign = "center";

  for (let i = 0; i < barData.length; i++) {
    let value = barData[i];
    let h = (value / max) * (c.height - 30);   // laisse de la place pour le texte
    let x = i * bw + 5;
    let barW = bw - 10;
    let y = c.height - h - 10;

    // barre
    ctx.fillStyle = value > 0 ? "#4f4" : "#333";
    ctx.fillRect(x, y, barW, h);

    // texte au-dessus de la barre
    ctx.fillStyle = "#fff";
    ctx.fillText(value, x + barW / 2, y - 5);
  }
}

function drawRadar() {
  let c = document.getElementById('radar');
  let ctx = c.getContext('2d');

  ctx.clearRect(0,0,c.width,c.height);

  let cx = c.width/2, cy = c.height/2;
  let r = c.width/2 - 10;

  ctx.strokeStyle="#444";
  ctx.beginPath(); ctx.arc(cx,cy,r,0,2*Math.PI); ctx.stroke();
  ctx.beginPath(); ctx.arc(cx,cy,r*0.6,0,2*Math.PI); ctx.stroke();

  radarAngle += 0.1;
  let x2 = cx + r * Math.cos(radarAngle);
  let y2 = cy + r * Math.sin(radarAngle);

  ctx.strokeStyle = lastValue ? '#f44' : '#4f4';
  ctx.beginPath(); ctx.moveTo(cx,cy); ctx.lineTo(x2,y2); ctx.stroke();

  if (lastValue) {
    ctx.fillStyle = '#f44';
    ctx.beginPath();
    ctx.arc(cx + r*0.6*Math.cos(radarAngle),
            cy + r*0.6*Math.sin(radarAngle),
            6,0,2*Math.PI);
    ctx.fill();
  }
}

function connectWS() {
  ws = new WebSocket("ws://" + location.hostname + ":81/");

  ws.onmessage = (e) => {
    let d = JSON.parse(e.data);

    lastValue = d.value;

    document.getElementById('status').textContent = d.status;
    document.getElementById('status').style.background =
      d.value ? '#f44' : '#4f4';

    document.getElementById('count').textContent = d.count;
    document.getElementById('alarm').textContent = d.alarm ? "OUI" : "NON";
      // Mise à jour du bouton Son en fonction de d.sound
  let sb = document.getElementById('soundBtn');
  if (d.sound) {
    sb.textContent = "Son ON";
    sb.style.background = "#2d5";   // vert
  } else {
    sb.textContent = "Son OFF";
    sb.style.background = "#555";   // gris
  }


    if (d.value == 1) {
      let m = new Date().getMinutes();
      if (m != lastMinute) {
        lastMinute = m;
        barData.unshift(0);
        barData.pop();
      }
      barData[0]++;
    }

    drawRadar();
    drawBar();
  };

  ws.onclose = () => setTimeout(connectWS, 1500);
}


window.onload = () => {
  connectWS();
  drawRadar();
  drawBar();
};

</script>

</body>
</html>
)=====";
  return html;
}

// =========================================================
//                     HANDLERS HTTP
// =========================================================
void handleRoot() {
  if (!server.authenticate(http_username, http_password)) {
    return server.requestAuthentication();
  }
  server.send(200, "text/html", webPage());
}

void handleReset() {
  if (!server.authenticate(http_username, http_password)) {
    return server.requestAuthentication();
  }
  detectCount = 0;
  server.send(200, "text/plain", "OK");
}

void handleSound() {
  if (!server.authenticate(http_username, http_password)) {
    return server.requestAuthentication();
  }
  soundOn = !soundOn;
  if (!soundOn) M5.Speaker.stop();
  wsSend();
  server.send(200, "text/plain", "OK");
}



// =========================================================
//                         SETUP
// =========================================================
void setup() {

  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Lcd.setRotation(0);

  Serial.begin(115200);
  pinMode(IR_PIN, INPUT_PULLUP);

  initLayout();
  drawDashboard("LOIN", GREEN);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connexion WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("\nConnecté !");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/reset", handleReset);
  server.on("/sound", handleSound);
  server.begin();
  Serial.println("Serveur HTTP démarré.");

  ws.onEvent(wsEvent);   // 🔥 Maintenant fonctionne
  ws.begin();
  Serial.println("WebSocket sur port 81 !");
}

// =========================================================
//                     WEBSOCKET SEND
// =========================================================
void wsSend() {
  String json = "{";
  json += "\"value\":" + String(stableDetect ? 1 : 0) + ",";
  json += "\"status\":\"" + String(stableDetect ? "PROCHE" : "LOIN") + "\",";
  json += "\"count\":" + String(detectCount) + ",";
  json += "\"alarm\":" + String(alarmOn ? 1 : 0) + ",";
  json += "\"sound\":" + String(soundOn ? 1 : 0);
  json += "}";

  ws.broadcastTXT(json);
}

// =========================================================
//                      WEBSOCKET EVENT
// =========================================================
void wsEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len) {
  if (type == WStype_CONNECTED) {
    Serial.printf("Client WebSocket #%u connecté\n", num);
    wsSend();
  }
}

// =========================================================
//                 HISTORIQUE LOCAL (10 derniers)
// =========================================================
void addHistory(bool val) {
  history[histPos].t = millis();
  history[histPos].val = val;

  histPos = (histPos + 1) % 10;
  if (histPos == 0) histFull = true;
}

// =========================================================
//                        LOOP
// =========================================================
void loop() {
  M5.update();
  server.handleClient();
  ws.loop();

  unsigned long now = millis();

  // -------- LECTURE CAPTEUR IR --------
  bool newRaw = (digitalRead(IR_PIN) == LOW);  // LOW = détection

  if (newRaw != rawDetect) {
    rawDetect = newRaw;
    lastChange = now;
  }

  // -------- FILTRAGE (sensibilité) --------
  if (rawDetect != stableDetect && (now - lastChange) >= sensitivity) {
    stableDetect = rawDetect;
    addHistory(stableDetect);

    if (stableDetect) {
      // Passage à PROCHES
      detectCount++;
      detectStart = now;
      alarmOn = false;
      drawDashboard("PROCHE", RED);
    } else {
      // Passage à LOIN
      alarmOn = false;
      M5.Speaker.stop();  // on coupe le son si on n'est plus proche
      drawDashboard("LOIN", GREEN);
    }

    wsSend();
  }

  // -------- ALARME AUTOMATIQUE (2 s) --------
  if (stableDetect && !alarmOn) {
    if (now - detectStart >= ALARM_MS) {
      alarmOn = true;
      drawDashboard("ALERTE", ORANGE);
      if (soundOn) {
        M5.Speaker.tone(1000, 200);  // bip 200 ms
        lastBeepMs = now;
      }
      wsSend();
    }
  }

  // Bip répété tant que l’alarme est active et le son ON
  if (alarmOn && soundOn) {
    if (now - lastBeepMs >= 500) {   // toutes les 500 ms
      M5.Speaker.tone(1000, 200);
      lastBeepMs = now;
    }
  }

  // -------- GRAPHIQUE TABLETTE --------
  if (now - lastGraphMs >= 150) {
    lastGraphMs = now;
    addGraphPoint(stableDetect);
    drawGraph();
  }

  // -------- GESTION TACTILE --------
  auto t = M5.Touch.getDetail();

  if (t.wasPressed()) {
    // Bouton RESET
    if (t.x >= BTN_RESET_X && t.x <= BTN_RESET_X + BTN_RESET_W &&
        t.y >= BTN_RESET_Y && t.y <= BTN_RESET_Y + BTN_RESET_H) {

      detectCount = 0;
      alarmOn = false;
      M5.Speaker.stop();

      drawDashboard(stableDetect ? "PROCHE" : "LOIN",
                    stableDetect ? RED : GREEN);

      wsSend();
    }

    // Bouton SON
    if (t.x >= BTN_SOUND_X && t.x <= BTN_SOUND_X + BTN_SOUND_W &&
        t.y >= BTN_SOUND_Y && t.y <= BTN_SOUND_Y + BTN_SOUND_H) {

      soundOn = !soundOn;
      if (!soundOn) {
        M5.Speaker.stop();   // coupe immédiatement le bip
      }

      drawDashboard(stableDetect ? "PROCHE" : "LOIN",
                    stableDetect ? RED : GREEN);

      wsSend();
    }

    // Slider de sensibilité
    if (t.x >= SLIDER_X && t.x <= SLIDER_X + SLIDER_W &&
        t.y >= SLIDER_Y - 20 && t.y <= SLIDER_Y + 40) {

      float ratio = float(t.x - SLIDER_X) / float(SLIDER_W);
      if (ratio < 0.0f) ratio = 0.0f;
      if (ratio > 1.0f) ratio = 1.0f;

      sensitivity = SENS_MIN + (unsigned long)(ratio * (SENS_MAX - SENS_MIN));

      drawDashboard(stableDetect ? "PROCHE" : "LOIN",
                    stableDetect ? RED : GREEN);

      wsSend();
    }
  }
}
