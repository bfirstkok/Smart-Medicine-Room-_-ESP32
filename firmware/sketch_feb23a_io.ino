#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// ✅ เพิ่มสำหรับตั้งค่า Wi-Fi แบบไม่ฝังในโค้ด
#include <WiFiManager.h>   // by tzapu
#include <WebServer.h>
#include <Preferences.h>

// ================== PIN CONFIG ==================
#define DHTPIN 27
#define DHTTYPE DHT22

const int IR_PIN     = 14;
const int RELAY_PIN  = 26;
const int LED_PIN    = 4;
const int BUZZER_PIN = 25;
const int FAN_SERVO_PIN  = 18;  // SG90 (ส่าย)
const int DOOR_SERVO_PIN = 19;  // MG90S (ประตู)

// ================== API (ฝังได้ ไม่ใช่ Wi-Fi) ==================
String BASE_URL = "http://172.20.10.4/iot/api";
String deviceId = "esp32_1";

// ================== ✅ SECURITY (เพิ่มใหม่) ==================
String API_KEY      = "FIRST_IOT_2026_SECRET";
String DEVICE_TOKEN = "ESP32_ROOM_A_TOKEN_999";

static inline void addSecurityHeaders(HTTPClient &http) {
  http.addHeader("x-api-key", API_KEY);
  http.addHeader("x-device-token", DEVICE_TOKEN);
}

// ================== OBJECTS ==================
DHT dht(DHTPIN, DHTTYPE);
Servo fanServo;
Servo doorServo;

// ================== LOCAL WEB (reset wifi) ==================
Preferences prefs;
WebServer localServer(80);
// ================== WIFI PORTAL (remote) ==================
WiFiManager wm;                  // ใช้ตัวเดียวทั้งโปรแกรม (สำคัญ)
bool portalActive = false;
unsigned long portalStartMs = 0;
const unsigned long PORTAL_TIMEOUT_MS = 180000; // 3 นาที

// ================== TIMERS ==================
unsigned long tSend = 0;
unsigned long tCmd  = 0;

// ================== RELAY LOGIC ==================
bool RELAY_ACTIVE_LOW = true;
bool relayManual = false;
int  relayManualVal = 0;
unsigned long lightUntil = 0;

void setRelay(int v) {
  if (RELAY_ACTIVE_LOW) digitalWrite(RELAY_PIN, v ? LOW : HIGH);
  else                  digitalWrite(RELAY_PIN, v ? HIGH : LOW);
}

// ================== LED MODE ==================
int ledMode = 0;
unsigned long tLed = 0;
bool ledState = false;

const int LED_FAST_DELAY = 150;
const int LED_ONESHOT_MS = 140;

// ================== BUZZER MODE ==================
int buzzerMode = 0;
unsigned long tBz = 0;
unsigned long bzOneShotUntil = 0;
bool bzState = false;

const int BUZZER_FAST_DELAY = 120;
const int BUZZER_ONESHOT_MS = 120;

// ================== IR EDGE DETECT (Active-Low) ==================
int prevIrRaw = HIGH;                 // ปกติไม่มีคน = HIGH
unsigned long tEntryNotify = 0;
const unsigned long ENTRY_COOLDOWN = 5000; // กันแจ้งซ้ำ 5 วิ

// ================== THRESHOLDS ==================
const float TEMP_MIN = 0.0;
const float TEMP_MAX = 30.0;
const float HUM_MIN  = 40.0;
const float HUM_MAX  = 80.0;

bool criticalMode = false;
unsigned long tCriticalNotify = 0;
const unsigned long CRITICAL_NOTIFY_COOLDOWN = 5000; // 5 วิ

// ================== LAST SENSOR CACHE ==================
float lastTemp = NAN;
float lastHum  = NAN;

// ================== SERVO STATE ==================
int doorTarget = 0;  // 0-90
int lastDoorSent = -999;

// ================== FAN (SG90 SWING MODE) ==================
bool fanEnabled  = true;
bool fanSwing    = true;
int  fanAngle    = 90;
int  fanMinAngle = 40;
int  fanMaxAngle = 140;
int  fanStep     = 2;
int  fanDir      = +1;
unsigned long tFan = 0;
const unsigned long FAN_STEP_DELAY = 20;

void runFanSwing() {
  if (!fanEnabled) return;

  unsigned long now = millis();
  if (now - tFan < FAN_STEP_DELAY) return;
  tFan = now;

  if (!fanSwing) return;

  fanAngle += fanDir * fanStep;
  if (fanAngle >= fanMaxAngle) { fanAngle = fanMaxAngle; fanDir = -1; }
  if (fanAngle <= fanMinAngle) { fanAngle = fanMinAngle; fanDir = +1; }

  fanServo.write(fanAngle);
}

// ================== WIFI MANAGER (ไม่ฝัง SSID/PASS) ==================
String prefGet(const char* key, const String& def) { return prefs.getString(key, def); }
void prefSet(const char* key, const String& val) { prefs.putString(key, val); }

void setupLocalEndpoints() {
  localServer.on("/", []() {
    String html = "<h2>ESP32 Online</h2>";
    html += "<p>IP: " + WiFi.localIP().toString() + "</p>";
    html += "<p>BASE_URL: " + BASE_URL + "</p>";
    html += "<p>deviceId: " + deviceId + "</p>";
    html += "<p><a href='/resetwifi'>Reset WiFi</a></p>";
    localServer.send(200, "text/html", html);
  });

  localServer.on("/resetwifi", []() {
    localServer.send(200, "text/plain", "Resetting WiFi settings...");

    WiFiManager wm;
    wm.resetSettings();

    prefs.begin("cfg", false);
    prefs.clear();
    prefs.end();

    delay(800);
    ESP.restart();
  });

  localServer.begin();
}

void setupWiFiNoHardcode() {
  WiFi.mode(WIFI_STA);

  // โหลดค่าเดิม (base_url/device_id) จาก NVS
  prefs.begin("cfg", true);
  BASE_URL = prefGet("base_url", BASE_URL);
  deviceId = prefGet("device_id", deviceId);
  prefs.end();

  wm.setConfigPortalTimeout(180);
  wm.setConnectTimeout(20);

  // ทำพารามิเตอร์ config
  char baseBuf[160];
  char idBuf[40];
  BASE_URL.toCharArray(baseBuf, sizeof(baseBuf));
  deviceId.toCharArray(idBuf, sizeof(idBuf));

  WiFiManagerParameter p_base("base", "BASE_URL (API)", baseBuf, 160);
  WiFiManagerParameter p_id("devid", "deviceId", idBuf, 40);

  wm.addParameter(&p_base);
  wm.addParameter(&p_id);

  // ✅ ตอนบูต: ใช้ BLOCKING MODE ให้ต่อ WiFi ให้ได้ก่อน
  wm.setConfigPortalBlocking(true);

  bool ok = wm.autoConnect("SMR-ESP32-SETUP");
  if (!ok) {
    Serial.println("[WiFi] Failed to connect -> restart");
    delay(1000);
    ESP.restart();
  }

  Serial.println("[WiFi] Connected!");
  Serial.println("IP: " + WiFi.localIP().toString());

  // เก็บค่าที่กรอกจาก portal
  BASE_URL = String(p_base.getValue());
  deviceId = String(p_id.getValue());

  prefs.begin("cfg", false);
  prefSet("base_url", BASE_URL);
  prefSet("device_id", deviceId);
  prefs.end();
}

// ================== TELEGRAM ==================
void telegramNotify(const String& type, float t, float h) {
  if (WiFi.status() != WL_CONNECTED) return;
  if (isnan(t) || isnan(h)) return;

  HTTPClient http;

  String url;
  if (type == "entry") url = BASE_URL + "/telegram_entry.php";
  else                 url = BASE_URL + "/telegram_critical.php";

  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  addSecurityHeaders(http);

  String body = "device_id=" + deviceId +
                "&temp_c=" + String(t, 1) +
                "&humid=" + String(h, 1);

  int code = http.POST(body);
  String resp = http.getString();
  http.end();

  Serial.println("[telegramNotify] url=" + url + " code=" + String(code) + " resp=" + resp);
}

void telegramCriticalNotify(float t, float h) { telegramNotify("critical", t, h); }
void telegramEntryNotify(float t, float h)    { telegramNotify("entry", t, h); }

// ================== ALERT TRIGGERS ==================
void triggerEntryAlert() {
  ledMode = 4;
  tLed = millis();
  digitalWrite(LED_PIN, HIGH);

  buzzerMode = 1;
  bzOneShotUntil = millis() + BUZZER_ONESHOT_MS;
  digitalWrite(BUZZER_PIN, HIGH);
}

void startCriticalAlert() {
  ledMode = 2;
  buzzerMode = 2;
  tLed = millis();
  tBz  = millis();
}

void stopCriticalAlertIfNeeded() {
  if (ledMode == 2) { ledMode = 0; digitalWrite(LED_PIN, LOW); }
  if (buzzerMode == 2) { buzzerMode = 0; digitalWrite(BUZZER_PIN, LOW); }
}

// ================== BACKGROUND RUNNERS ==================
void runLedMode() {
  unsigned long now = millis();

  if (ledMode == 0) { digitalWrite(LED_PIN, LOW); return; }
  if (ledMode == 1) { digitalWrite(LED_PIN, HIGH); return; }

  if (ledMode == 4) {
    if (now - tLed >= LED_ONESHOT_MS) { digitalWrite(LED_PIN, LOW); ledMode = 0; }
    return;
  }

  if (ledMode == 2) {
    if (now - tLed >= LED_FAST_DELAY) {
      tLed = now;
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    }
    return;
  }
}

void runBuzzerMode() {
  unsigned long now = millis();

  if (buzzerMode == 0) { digitalWrite(BUZZER_PIN, LOW); return; }

  if (buzzerMode == 1) {
    if (now < bzOneShotUntil) digitalWrite(BUZZER_PIN, HIGH);
    else { digitalWrite(BUZZER_PIN, LOW); buzzerMode = 0; }
    return;
  }

  if (buzzerMode == 2) {
    if (now - tBz >= BUZZER_FAST_DELAY) {
      tBz = now;
      bzState = !bzState;
      digitalWrite(BUZZER_PIN, bzState ? HIGH : LOW);
    }
    return;
  }
}

// ================== SENSOR + CRITICAL CHECK ==================
void checkCriticalAndAlert(float t, float h) {
  bool tempBad = (t < TEMP_MIN || t > TEMP_MAX);
  bool humBad  = (h < HUM_MIN  || h > HUM_MAX);
  bool newCritical = (tempBad || humBad);

  unsigned long now = millis();

  if (newCritical) {
    if (!criticalMode) {
      criticalMode = true;
      startCriticalAlert();
      tCriticalNotify = now;
      telegramCriticalNotify(t, h);
      return;
    }

    startCriticalAlert();
    if (now - tCriticalNotify >= CRITICAL_NOTIFY_COOLDOWN) {
      tCriticalNotify = now;
      telegramCriticalNotify(t, h);
    }
  } else {
    if (criticalMode) {
      criticalMode = false;
      stopCriticalAlertIfNeeded();
    }
  }
}

// ================== SEND SENSOR ==================
void sendSensor() {
  if (WiFi.status() != WL_CONNECTED) return;

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("[sendSensor] DHT read failed (NaN)");
    return;
  }

  lastTemp = t;
  lastHum  = h;

  checkCriticalAndAlert(t, h);

  int irRaw = digitalRead(IR_PIN);
  int irState = (irRaw == LOW) ? 1 : 0;

  HTTPClient http;
  String url = BASE_URL + "/sensor_push.php";
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  addSecurityHeaders(http);

  String body = "device_id=" + deviceId +
                "&temp_c=" + String(t, 1) +
                "&humid=" + String(h, 1) +
                "&ir_state=" + String(irState);

  int code = http.POST(body);
  String resp = http.getString();
  http.end();

  Serial.println("[sendSensor] code=" + String(code) + " resp=" + resp);
}

// ================== COMMAND DONE ==================
void cmdDone(int id, String cmd, String value) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = BASE_URL + "/cmd_done.php";
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  addSecurityHeaders(http);

  String body = "id=" + String(id) +
                "&device_id=" + deviceId +
                "&cmd=" + cmd +
                "&value=" + value;

  int code = http.POST(body);
  String resp = http.getString();
  http.end();

  Serial.println("[cmdDone] code=" + String(code) + " resp=" + resp);
}

// ================== APPLY COMMAND ==================
void applyCommand(String cmd, String value) {
  cmd.trim();
  value.trim();
  cmd.toUpperCase();

  Serial.println(">>> APPLY CMD: " + cmd + " = " + value);

  if (cmd == "RELAY") {
    int v = value.toInt();
    relayManual = true;
    relayManualVal = (v == 1) ? 1 : 0;
    return;
  }
  if (cmd == "RELAY_AUTO") { relayManual = false; return; }

  if (cmd == "LED") { ledMode = value.toInt(); tLed = millis(); return; }

  if (cmd == "BUZZER") {
    int v = value.toInt();
    buzzerMode = 0;
    digitalWrite(BUZZER_PIN, v ? HIGH : LOW);
    return;
  }

  if (cmd == "FAN") {
    int v = value.toInt();
    fanEnabled = (v == 1);
    if (!fanEnabled) {
      fanSwing = false;
      fanAngle = 90;
      fanServo.write(90);
    } else {
      fanSwing = true;
    }
    return;
  }

  if (cmd == "FAN_SPEED") {
    int sp = constrain(value.toInt(), 1, 10);
    fanStep = sp;
    return;
  }

  // DOOR: 0=เปิด, 90=ปิด (สลับมุม)
  if (cmd == "DOOR") {
    int v = constrain(value.toInt(), 0, 90);
    doorTarget = 90 - v;
    doorServo.write(doorTarget);
    return;
  }
  // ================== WIFI PORTAL (remote) ==================
    // ================== WIFI PORTAL (remote) ==================
  if (cmd == "WIFI_PORTAL") {
    if (!portalActive) {
      portalActive = true;
      portalStartMs = millis();

      // เปิด portal แบบ non-blocking (ต้องมี wm.process() ใน loop)
      wm.setConfigPortalBlocking(false);
      wm.setConfigPortalTimeout(180);

      bool started = wm.startConfigPortal("SMR-ESP32-SETUP");
      Serial.println(started ? "[WIFI] Portal started: SMR-ESP32-SETUP"
                             : "[WIFI] Portal start failed");

      // หมายเหตุ: ตอน portal ทำงาน ESP จะเป็น AP/STA สลับได้
      // ถ้า API จะยิงไม่ได้ในช่วงนี้เป็นเรื่องปกติ เพราะมันอาจหลุดจาก WiFi เดิม
    } else {
      Serial.println("[WIFI] Portal already active");
    }
    return;
  }

  if (cmd == "WIFI_RESET") {
    Serial.println("[WIFI] Reset settings + restart");

    // ถ้า portal ค้างอยู่ ปิดก่อน
    wm.stopConfigPortal();

    wm.resetSettings();     // ล้าง SSID/PASS

    prefs.begin("cfg", false);
    prefs.clear();          // ล้าง base_url/device_id
    prefs.end();

    delay(800);
    ESP.restart();
    return;
  }
}

// ================== FETCH COMMAND ==================
void fetchCommand() {
  if (WiFi.status() != WL_CONNECTED) return;

  String url = BASE_URL + "/cmd_get.php?device_id=" + deviceId;

  HTTPClient http;
  http.setTimeout(4000);
  http.begin(url);
  addSecurityHeaders(http);

  int code = http.GET();
  String payload = http.getString();
  http.end();

  if (code != 200) return;
  if (payload.length() == 0) return;

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) return;

  bool hasCmd = doc.containsKey("has_cmd") ? (bool)doc["has_cmd"] : doc.containsKey("cmd");
  if (!hasCmd) return;

  int id = doc["id"] | 0;
  String cmd = String((const char*)doc["cmd"]);
  String value = String((const char*)doc["value"]);

  Serial.printf("[CMD] id=%d cmd=%s value=%s\n", id, cmd.c_str(), value.c_str());

  applyCommand(cmd, value);
  cmdDone(id, cmd, value);
}

// ================== ENTRY (ยังใช้เพื่อ People Entry วันนี้/ทั้งหมด) ==================
void pushEntryEvent() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = BASE_URL + "/entry_push.php";
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  addSecurityHeaders(http);

  String body = "device_id=" + deviceId;

  int code = http.POST(body);
  String resp = http.getString();
  http.end();

  Serial.println("[pushEntryEvent] code=" + String(code) + " resp=" + resp);
}
void ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.println("[WiFi] reconnecting...");
  WiFi.mode(WIFI_STA);
  WiFi.reconnect();

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(200);
  }

  Serial.println("[WiFi] status=" + String((int)WiFi.status()) +
                 " ip=" + WiFi.localIP().toString());
}
// ================== SETUP / LOOP ==================
void setup() {
  Serial.begin(115200);

  pinMode(IR_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  setRelay(0);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  // Timer สำหรับ ESP32Servo
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  fanServo.setPeriodHertz(50);
  doorServo.setPeriodHertz(50);

  fanServo.attach(FAN_SERVO_PIN, 500, 2400);
  doorServo.attach(DOOR_SERVO_PIN, 500, 2400);

  fanAngle = 90;
  fanServo.write(fanAngle);

  doorTarget = 0;
  doorServo.write(doorTarget);
  lastDoorSent = doorTarget;

  dht.begin();

  setupWiFiNoHardcode();
  setupLocalEndpoints();

  prevIrRaw = digitalRead(IR_PIN);
}

void loop() {
  unsigned long now = millis();

  localServer.handleClient();
  // ===== WiFiManager process (ทำให้ portal ทำงานได้) =====
  wm.process();
if (portalActive && WiFi.status() == WL_CONNECTED) {
  portalActive = false;
  Serial.println("[WIFI] Portal finished, back online");
}
  

  if (portalActive) {
    // ถ้าหมดเวลา ให้รีสตาร์ท (กัน portal ค้าง)
    if (millis() - portalStartMs > PORTAL_TIMEOUT_MS) {
      Serial.println("[WIFI] Portal timeout -> restart");
      portalActive = false;
      ESP.restart();
    }
  }
  // ===== IR entry detect: แจ้งทันทีเมื่อมีคนเข้า (HIGH->LOW) =====
  int irRaw = digitalRead(IR_PIN);
  bool irEdgeDetected = (prevIrRaw == HIGH && irRaw == LOW);
  prevIrRaw = irRaw;

  if (irEdgeDetected && (millis() - tEntryNotify > ENTRY_COOLDOWN)) {
    tEntryNotify = millis();

    // ✅ (ตัดแล้ว) ไม่ส่ง ir_event_add.php อีก
    triggerEntryAlert();
    pushEntryEvent();

    float t = lastTemp;
    float h = lastHum;
    if (isnan(t) || isnan(h)) {
      t = dht.readTemperature();
      h = dht.readHumidity();
      if (!isnan(t) && !isnan(h)) { lastTemp = t; lastHum = h; }
    }
    telegramEntryNotify(t, h);

    lightUntil = now + 10000;
  }

  // ===== Relay behavior =====
  if (relayManual) setRelay(relayManualVal);
  else            setRelay(now < lightUntil ? 1 : 0);

  // ===== periodic tasks =====
  if (now - tSend >= 5000) { tSend = now; sendSensor(); }
  if (now - tCmd  >= 1000) { tCmd  = now; fetchCommand(); }

  runFanSwing();

  // ===== door update (only when changed) =====
  if (doorTarget != lastDoorSent) {
    doorServo.write(doorTarget);
    lastDoorSent = doorTarget;
    Serial.println("[SERVO] doorTarget -> " + String(doorTarget));
  }

  runLedMode();
  runBuzzerMode();
}
