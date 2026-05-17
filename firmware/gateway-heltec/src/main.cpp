#include <Arduino.h>

#include <WiFi.h>
#include <Preferences.h>

#include <LittleFS.h>

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

#include <LoRa.h>

#include "config.h"

struct TankConfig {
  uint8_t id = 0;
  String name;
  float capacityL = 0;
  float usePerDayL = 0;
};

struct TankState {
  TankConfig cfg;
  bool online = false;
  float levelPct = 0;
  float battV = 0;
  uint8_t battPct = 0;
  uint32_t lastSeenMs = 0;
};

struct __attribute__((packed)) TankPacket {
  uint8_t tankId;
  uint32_t uptimeS;
  uint16_t levelPct_x100;
  uint16_t battV_mV;
  uint8_t battPct;
  uint16_t crc;
};

static uint16_t checksum16(const uint8_t* data, size_t n) {
  uint16_t s = 0;
  for (size_t i = 0; i < n; i++) s = (uint16_t)(s + data[i]);
  return s;
}

static Preferences prefs;
static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

static String g_gatewayName = "Heltec";
static uint8_t g_tankCount = 2;
static uint32_t g_offlineTimeoutS = DEFAULT_OFFLINE_TIMEOUT_S;
static String g_loraKey = "";
static TankState g_tanks[8];

static void loadDefaults() {
  for (uint8_t i = 0; i < 8; i++) {
    g_tanks[i].cfg.id = i + 1;
    g_tanks[i].cfg.name = String("Tank ") + String(i + 1);
    g_tanks[i].cfg.capacityL = 0;
    g_tanks[i].cfg.usePerDayL = 0;
  }
}

static void loadConfig() {
  prefs.begin("tankmon", true);
  g_gatewayName = prefs.getString("gwName", "Heltec");
  g_tankCount = (uint8_t)prefs.getUChar("count", 2);
  if (g_tankCount < 1) g_tankCount = 1;
  if (g_tankCount > 8) g_tankCount = 8;
  g_offlineTimeoutS = prefs.getUInt("offS", DEFAULT_OFFLINE_TIMEOUT_S);
  if (g_offlineTimeoutS < 5) g_offlineTimeoutS = 5;
  g_loraKey = prefs.getString("lkey", "");

  for (uint8_t i = 0; i < 8; i++) {
    char key[16];
    snprintf(key, sizeof(key), "n%u", i + 1);
    g_tanks[i].cfg.name = prefs.getString(key, g_tanks[i].cfg.name);
    snprintf(key, sizeof(key), "c%u", i + 1);
    g_tanks[i].cfg.capacityL = prefs.getFloat(key, 0);
    snprintf(key, sizeof(key), "u%u", i + 1);
    g_tanks[i].cfg.usePerDayL = prefs.getFloat(key, 0);
  }
  prefs.end();
}

static void saveConfig() {
  prefs.begin("tankmon", false);
  prefs.putString("gwName", g_gatewayName);
  prefs.putUChar("count", g_tankCount);
  prefs.putUInt("offS", g_offlineTimeoutS);
  prefs.putString("lkey", g_loraKey);
  for (uint8_t i = 0; i < 8; i++) {
    char key[16];
    snprintf(key, sizeof(key), "n%u", i + 1);
    prefs.putString(key, g_tanks[i].cfg.name);
    snprintf(key, sizeof(key), "c%u", i + 1);
    prefs.putFloat(key, g_tanks[i].cfg.capacityL);
    snprintf(key, sizeof(key), "u%u", i + 1);
    prefs.putFloat(key, g_tanks[i].cfg.usePerDayL);
  }
  prefs.end();
}

static float calcTimeToEmptyH(const TankState& t) {
  if (t.cfg.capacityL <= 0.0f) return NAN;
  if (t.cfg.usePerDayL <= 0.0f) return NAN;
  const float litersLeft = (t.levelPct / 100.0f) * t.cfg.capacityL;
  const float days = litersLeft / t.cfg.usePerDayL;
  return days * 24.0f;
}

static String jsonEscape(const String& s) {
  String out;
  out.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    const char c = s[i];
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else out += c;
  }
  return out;
}

static String buildStateJson() {
  const uint32_t now = millis();
  String s;
  s.reserve(1024);
  s += "{\"type\":\"state\",\"gatewayName\":\"" + jsonEscape(g_gatewayName) + "\",\"tanks\":[";
  for (uint8_t i = 0; i < g_tankCount; i++) {
    if (i) s += ",";
    TankState& t = g_tanks[i];
    const bool online = t.lastSeenMs && ((now - t.lastSeenMs) <= (g_offlineTimeoutS * 1000UL));
    t.online = online;
    const float tteH = calcTimeToEmptyH(t);

    s += "{";
    s += "\"id\":" + String(t.cfg.id) + ",";
    s += "\"name\":\"" + jsonEscape(t.cfg.name) + "\",";
    s += "\"online\":" + String(online ? "true" : "false") + ",";
    s += "\"levelPct\":" + String(t.levelPct, 2) + ",";
    s += "\"battV\":" + String(t.battV, 3) + ",";
    s += "\"battPct\":" + String(t.battPct) + ",";
    s += "\"lastSeenMs\":" + String((uint32_t)t.lastSeenMs) + ",";
    if (isnan(tteH)) s += "\"timeToEmptyH\":null";
    else s += "\"timeToEmptyH\":" + String(tteH, 2);
    s += "}";
  }
  s += "]}";
  return s;
}

static String buildConfigJson() {
  String s;
  s.reserve(1024);
  s += "{";
  s += "\"gatewayName\":\"" + jsonEscape(g_gatewayName) + "\",";
  s += "\"tankCount\":" + String(g_tankCount) + ",";
  s += "\"offlineTimeoutS\":" + String(g_offlineTimeoutS) + ",";
  s += "\"loraKey\":\"" + jsonEscape(g_loraKey) + "\",";
  s += "\"tanks\":[";
  for (uint8_t i = 0; i < g_tankCount; i++) {
    if (i) s += ",";
    const TankConfig& c = g_tanks[i].cfg;
    s += "{";
    s += "\"id\":" + String(c.id) + ",";
    s += "\"name\":\"" + jsonEscape(c.name) + "\",";
    s += "\"capacityL\":" + String(c.capacityL, 3) + ",";
    s += "\"usePerDayL\":" + String(c.usePerDayL, 3);
    s += "}";
  }
  s += "]}";
  return s;
}

static void wsBroadcastState() {
  const String msg = buildStateJson();
  ws.textAll(msg);
}

static void onWsEvent(AsyncWebSocket* server_, AsyncWebSocketClient* client, AwsEventType type,
                      void* arg, uint8_t* data, size_t len) {
  (void)server_;
  (void)arg;
  (void)data;
  (void)len;
  if (type == WS_EVT_CONNECT) {
    client->text(buildStateJson());
  }
}

static void setupWeb() {
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "application/json", buildStateJson());
  });

  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "application/json", buildConfigJson());
  });

  server.on(
      "/api/config", HTTP_POST,
      [](AsyncWebServerRequest* req) {
        // response handled in body handler
        (void)req;
      },
      nullptr,
      [](AsyncWebServerRequest* req, uint8_t* body, size_t len, size_t index, size_t total) {
        // Simple JSON parsing (minimal) to avoid extra dependencies.
        // Expected keys: tankCount, offlineTimeoutS, loraKey, tanks[{id,name,capacityL,usePerDayL}]
        (void)index;
        (void)total;
        String b;
        b.reserve(len + 1);
        for (size_t i = 0; i < len; i++) b += (char)body[i];

        auto findNumber = [&](const char* key, double fallback) -> double {
          int p = b.indexOf(String("\"") + key + "\"");
          if (p < 0) return fallback;
          p = b.indexOf(':', p);
          if (p < 0) return fallback;
          int e = p + 1;
          while (e < (int)b.length() && (b[e] == ' ')) e++;
          int end = e;
          while (end < (int)b.length() && (isDigit(b[end]) || b[end] == '.' || b[end] == '-')) end++;
          return b.substring(e, end).toDouble();
        };

        auto findString = [&](const char* key, const String& fallback) -> String {
          int p = b.indexOf(String("\"") + key + "\"");
          if (p < 0) return fallback;
          p = b.indexOf(':', p);
          if (p < 0) return fallback;
          p = b.indexOf('"', p);
          if (p < 0) return fallback;
          int q = b.indexOf('"', p + 1);
          if (q < 0) return fallback;
          return b.substring(p + 1, q);
        };

        uint8_t newCount = (uint8_t)findNumber("tankCount", g_tankCount);
        if (newCount < 1) newCount = 1;
        if (newCount > 8) newCount = 8;

        g_tankCount = newCount;
        g_offlineTimeoutS = (uint32_t)findNumber("offlineTimeoutS", g_offlineTimeoutS);
        if (g_offlineTimeoutS < 5) g_offlineTimeoutS = 5;
        g_loraKey = findString("loraKey", g_loraKey);

        // Parse tanks array very simply.
        for (uint8_t i = 0; i < g_tankCount; i++) {
          // locate "id":<i+1>
          const String needle = String("\"id\":") + String(i + 1);
          int p = b.indexOf(needle);
          if (p < 0) continue;

          // name
          int npos = b.indexOf("\"name\"", p);
          if (npos > 0) {
            int c = b.indexOf(':', npos);
            int q1 = b.indexOf('"', c);
            int q2 = (q1 > 0) ? b.indexOf('"', q1 + 1) : -1;
            if (q1 > 0 && q2 > q1) g_tanks[i].cfg.name = b.substring(q1 + 1, q2);
          }

          // capacityL
          int cpos = b.indexOf("\"capacityL\"", p);
          if (cpos > 0) {
            int c = b.indexOf(':', cpos);
            int e = c + 1;
            while (e < (int)b.length() && (b[e] == ' ')) e++;
            int end = e;
            while (end < (int)b.length() && (isDigit(b[end]) || b[end] == '.' || b[end] == '-')) end++;
            g_tanks[i].cfg.capacityL = b.substring(e, end).toFloat();
          }

          // usePerDayL
          int upos = b.indexOf("\"usePerDayL\"", p);
          if (upos > 0) {
            int c = b.indexOf(':', upos);
            int e = c + 1;
            while (e < (int)b.length() && (b[e] == ' ')) e++;
            int end = e;
            while (end < (int)b.length() && (isDigit(b[end]) || b[end] == '.' || b[end] == '-')) end++;
            g_tanks[i].cfg.usePerDayL = b.substring(e, end).toFloat();
          }
        }

        saveConfig();
        wsBroadcastState();
        req->send(200, "application/json", "{\"ok\":true}");
      });

  // Static UI
  // Requires PlatformIO "Upload File System Image" (LittleFS)
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.onNotFound([](AsyncWebServerRequest* req) {
    req->send(404, "text/plain", "Not found");
  });

  server.begin();
}

static bool parsePacket(int packetLen, TankPacket& outPacket) {
  if (packetLen <= 0) return false;

  // Read key prefix
  String key;
  while (LoRa.available()) {
    const int c = LoRa.read();
    if (c < 0) break;
    if (c == 0) break;
    key += (char)c;
    if (key.length() > 32) break;
  }

  if (g_loraKey.length() && key != g_loraKey) {
    // ignore other systems
    return false;
  }

  if (LoRa.available() < (int)sizeof(TankPacket)) return false;
  uint8_t buf[sizeof(TankPacket)];
  for (size_t i = 0; i < sizeof(TankPacket); i++) {
    int c = LoRa.read();
    if (c < 0) return false;
    buf[i] = (uint8_t)c;
  }
  memcpy(&outPacket, buf, sizeof(TankPacket));

  const uint16_t expect = checksum16(buf, sizeof(TankPacket) - sizeof(outPacket.crc));
  if (expect != outPacket.crc) return false;
  if (outPacket.tankId < 1 || outPacket.tankId > 8) return false;
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  loadDefaults();
  loadConfig();

  // WiFi AP for tablet
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // LoRa
  if (!LoRa.begin(LORA_FREQ_HZ)) {
    Serial.println("LoRa init failed");
    while (true) delay(1000);
  }
  Serial.println("LoRa RX ready");

  // FS + web
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
  }
  setupWeb();
}

void loop() {
  const int packetLen = LoRa.parsePacket();
  if (packetLen) {
    TankPacket p{};
    if (parsePacket(packetLen, p)) {
      const uint8_t idx = (uint8_t)(p.tankId - 1);
      if (idx < 8) {
        TankState& t = g_tanks[idx];
        t.levelPct = (float)p.levelPct_x100 / 100.0f;
        t.battV = (float)p.battV_mV / 1000.0f;
        t.battPct = p.battPct;
        t.lastSeenMs = millis();
        t.online = true;
        wsBroadcastState();
        Serial.printf("RX tank=%u level=%.1f batt=%.2fV (%u%%)\n", p.tankId, t.levelPct, t.battV, t.battPct);
      }
    }
  }

  static uint32_t lastWs = 0;
  const uint32_t now = millis();
  if (now - lastWs >= WS_STATE_PERIOD_MS) {
    lastWs = now;
    ws.cleanupClients();
    wsBroadcastState();
  }
}
