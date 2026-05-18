#include <Arduino.h>

#include <WiFi.h>
#include <esp_now.h>
#include <Preferences.h>

#include <Wire.h>
#include <U8g2lib.h>

#include <LittleFS.h>

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

#include <LoRa.h>

#if __has_include("config.h")
#include "config.h"
#elif __has_include("include/config.h")
#include "include/config.h"
#else
#error "config.h not found"
#endif

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

#if USE_OLED
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);
#endif

static String g_gatewayName = "Heltec";
static uint8_t g_tankCount = 2;
static uint32_t g_offlineTimeoutS = DEFAULT_OFFLINE_TIMEOUT_S;
static String g_loraKey = "";
static TankState g_tanks[8];
static bool g_loraOk = false;

static const char* contentTypeForPath(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".css")) return "text/css";
  if (path.endsWith(".js")) return "application/javascript";
  if (path.endsWith(".json")) return "application/json";
  if (path.endsWith(".png")) return "image/png";
  if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
  if (path.endsWith(".svg")) return "image/svg+xml";
  if (path.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

static uint8_t detectOledAddress() {
  const uint8_t candidates[] = {0x3C, 0x3D};
  for (uint8_t addr : candidates) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      return addr;
    }
  }
  return OLED_I2C_ADDR;
}

static void drawStatus() {
#if USE_OLED
  const uint32_t now = millis();
  uint8_t onlineCount = 0;
  float maxLevel = 0;
  
  for (uint8_t i = 0; i < g_tankCount; i++) {
    const TankState& t = g_tanks[i];
    if (t.lastSeenMs && (now - t.lastSeenMs) <= (g_offlineTimeoutS * 1000UL)) {
      onlineCount++;
      if (t.levelPct > maxLevel) maxLevel = t.levelPct;
    }
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  
  // Title
  u8g2.drawStr(0, 12, "Tank Monitor");
  
  // Online status
  u8g2.setCursor(0, 26);
  u8g2.printf("Tanks: %u/%u online", onlineCount, g_tankCount);
  
  // WiFi AP info
  u8g2.setCursor(0, 40);
  u8g2.printf("AP: %s", AP_SSID);
  
  // IP address
  u8g2.setCursor(0, 54);
  u8g2.printf("IP: %s", WiFi.softAPIP().toString().c_str());
  
  // Tank status (show first tank on second line if connected)
  u8g2.setFont(u8g2_font_5x7_tr);
  if (g_tankCount > 0) {
    const TankState& t = g_tanks[0];
    const bool online = t.lastSeenMs && ((now - t.lastSeenMs) <= (g_offlineTimeoutS * 1000UL));
    u8g2.setCursor(0, 62);
    if (online) {
      u8g2.printf("T1: %.0f%% %uV", t.levelPct, (uint16_t)t.battV);
    } else {
      u8g2.drawStr(0, 62, "T1: offline");
    }
  }
  
  u8g2.sendBuffer();
#endif
}

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

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (LittleFS.exists("/index.html")) {
      req->send(LittleFS, "/index.html", "text/html");
      return;
    }
    // Fallback if index.html is missing
    String html = "<html><body><h1>Tank Monitor Gateway</h1><p>Loading...</p>";
    html += "<script>fetch('/api/state').then(r=>r.json()).then(d=>document.body.innerHTML='<h1>'+d.gatewayName+'</h1><p>Tanks online: '+d.tanks.filter(t=>t.online).length+'/'+d.tanks.length+'</p>');</script>";
    html += "</body></html>";
    req->send(200, "text/html", html);
  });

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
    String path = req->url();
    if (path.endsWith("/")) {
      path += "index.html";
    }
    String altPath = path.startsWith("/") ? path.substring(1) : "/" + path;
    if (LittleFS.exists(path) || LittleFS.exists(altPath)) {
      if (!LittleFS.exists(path)) {
        path = altPath;
      }
      req->send(LittleFS, path, contentTypeForPath(path));
      return;
    }
    Serial.printf("HTTP 404: %s\n", req->url().c_str());
    req->send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("Web server started on port 80");
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

static void onEspNowRecv(const uint8_t* mac, const uint8_t* data, int len) {
  if (len != (int)sizeof(TankPacket)) return;
  
  TankPacket p;
  memcpy(&p, data, sizeof(TankPacket));
  
  const uint16_t expect = checksum16(data, len - sizeof(p.crc));
  if (expect != p.crc) return;
  if (p.tankId < 1 || p.tankId > 8) return;
  
  const uint8_t idx = p.tankId - 1;
  TankState& t = g_tanks[idx];
  t.levelPct = (float)p.levelPct_x100 / 100.0f;
  t.battV = (float)p.battV_mV / 1000.0f;
  t.battPct = p.battPct;
  t.lastSeenMs = millis();
  t.online = true;
  wsBroadcastState();
  Serial.printf("RX (ESP-NOW) tank=%u level=%.1f batt=%.2fV (%u%%)\n", p.tankId, t.levelPct, t.battV, p.battPct);
  drawStatus();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n\n=== Gateway Starting ===");

  loadDefaults();
  loadConfig();

  // Print gateway MAC address for ESP-NOW pairing
  uint8_t macAddr[6];
  esp_read_mac(macAddr, ESP_MAC_WIFI_STA);
  Serial.print("Gateway MAC (for ESP-NOW): ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", macAddr[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  Serial.printf("Gateway MAC hex: {0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X}\n", 
    macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);

#if USE_OLED
  Serial.println("Initializing OLED...");
  if (OLED_VEXT_PIN >= 0) {
    pinMode(OLED_VEXT_PIN, OUTPUT);
    digitalWrite(OLED_VEXT_PIN, LOW);
    delay(100);
  }

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);
  Serial.println("I2C initialized");
  
  const uint8_t oledAddr = detectOledAddress();
  Serial.printf("OLED detected at 0x%02X\n", oledAddr);
  
  u8g2.setI2CAddress(oledAddr << 1);
  u8g2.begin();
  Serial.println("OLED initialized");
  
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 12, "Gateway init...");
  u8g2.sendBuffer();
#else
  Serial.println("OLED disabled");
#endif

  // WiFi AP for tablet
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP SSID: ");
  Serial.println(AP_SSID);
  Serial.print("AP Pass: ");
  Serial.println(AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("AP MAC: ");
  Serial.println(WiFi.softAPmacAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
  } else {
    Serial.println("ESP-NOW initialized");
    esp_now_register_recv_cb(onEspNowRecv);
  }

  // LoRa
#if defined(LORA_SCK) && defined(LORA_MISO) && defined(LORA_MOSI) && defined(LORA_SS)
  Serial.printf("LoRa freq: %.0f Hz\n", (double)LORA_FREQ_HZ);
  Serial.printf("LoRa pins: SCK=%d MISO=%d MOSI=%d SS=%d", LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
#if defined(LORA_RST) && defined(LORA_DIO0)
  Serial.printf(" RST=%d DIO0=%d", LORA_RST, LORA_DIO0);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
#else
  LoRa.setPins(LORA_SS, -1, -1);
#endif
  Serial.println();
#else
  Serial.printf("LoRa freq: %.0f Hz\n", (double)LORA_FREQ_HZ);
  Serial.printf("SPI pins: SCK=%d MISO=%d MOSI=%d SS=%d\n", SCK, MISO, MOSI, SS);
#endif
  g_loraOk = LoRa.begin(LORA_FREQ_HZ);
  if (!g_loraOk) {
    Serial.println("LoRa init failed (continuing without LoRa)");
  } else {
    Serial.println("LoRa RX ready");
  }

  // FS + web
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
  } else {
    Serial.println("LittleFS files:");
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while (file) {
      Serial.printf("  %s (%u bytes)\n", file.name(), (unsigned)file.size());
      file = root.openNextFile();
    }
  }
  Serial.println("Setting up web server...");
  setupWeb();
  Serial.println("Web server setup complete!");
  drawStatus();
}

void loop() {
  if (g_loraOk) {
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
          drawStatus();
        }
      }
    }
  }

  static uint32_t lastWs = 0;
  const uint32_t now = millis();
  if (now - lastWs >= WS_STATE_PERIOD_MS) {
    lastWs = now;
    ws.cleanupClients();
    wsBroadcastState();
    drawStatus();
  }
}
