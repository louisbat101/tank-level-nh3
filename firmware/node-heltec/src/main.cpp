#include <Arduino.h>
#include <LoRa.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <U8g2lib.h>

#include "config.h"

struct __attribute__((packed)) TankPacket {
  uint8_t tankId;
  uint32_t uptimeS;
  uint16_t levelPct_x100;
  uint16_t battV_mV;
  uint8_t battPct;
  uint16_t crc;
};

static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);
static WebServer server(80);
static IPAddress apIp;

static uint16_t checksum16(const uint8_t* data, size_t n) {
  uint16_t s = 0;
  for (size_t i = 0; i < n; i++) s = (uint16_t)(s + data[i]);
  return s;
}

static float clamp01(float x){ return x < 0 ? 0 : (x > 1 ? 1 : x); }

static float readAdcV(uint8_t pin){
  uint16_t raw = analogRead(pin);
  return (raw / ADC_MAX) * ADC_REF_V;
}

static float levelPctFromGaugeV(float v){
  float t = (v - GAUGE_V_EMPTY) / (GAUGE_V_FULL - GAUGE_V_EMPTY);
  return clamp01(t) * 100.0f;
}

static uint8_t battPctFromV(float vbatt){
  float t = (vbatt - 3.2f) / (4.2f - 3.2f);
  return (uint8_t)roundf(clamp01(t) * 100.0f);
}

static void drawStatus(float levelPct, float battV, uint8_t battPct) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 12, "Tank Node");
  u8g2.setCursor(0, 26);
  u8g2.printf("ID: %u", (unsigned)TANK_ID);
  u8g2.setCursor(0, 40);
  u8g2.printf("Level: %.0f%%", levelPct);
  u8g2.setCursor(0, 54);
  u8g2.printf("Batt: %.2fV %u%%", battV, battPct);
  u8g2.sendBuffer();
}

static void ledWrite(uint8_t pin, bool on) {
#if LED_ACTIVE_HIGH
  digitalWrite(pin, on ? HIGH : LOW);
#else
  digitalWrite(pin, on ? LOW : HIGH);
#endif
}

static void blinkLed(uint8_t pin, uint16_t ms) {
  ledWrite(pin, true);
  delay(ms);
  ledWrite(pin, false);
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

void setup(){
  Serial.begin(115200);
  delay(200);

  analogReadResolution(12);

  if (OLED_VEXT_PIN >= 0) {
    pinMode(OLED_VEXT_PIN, OUTPUT);
    digitalWrite(OLED_VEXT_PIN, LOW);
    delay(100);
  }

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);
  const uint8_t oledAddr = detectOledAddress();
  u8g2.setI2CAddress(oledAddr << 1);
  u8g2.begin();
  Serial.printf("OLED I2C addr: 0x%02X\n", oledAddr);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 12, "OLED init...");
  u8g2.sendBuffer();

  pinMode(PIN_LED_POWER, OUTPUT);
  pinMode(PIN_LED_COMMS, OUTPUT);
  ledWrite(PIN_LED_COMMS, false);
  ledWrite(PIN_LED_POWER, true);

  if (!LoRa.begin(LORA_FREQ_HZ)) {
    Serial.println("LoRa init failed");
    while (true) delay(1000);
  }

  LoRa.setTxPower(17);
  Serial.println("Heltec tank node started");

  // WiFi AP + status page
  WiFi.mode(WIFI_AP);
  String ssid = String(NODE_AP_SSID_PREFIX) + String(TANK_ID);
  WiFi.softAP(ssid.c_str(), NODE_AP_PASS);
  apIp = WiFi.softAPIP();
  Serial.print("Node AP IP: ");
  Serial.println(apIp);

  server.on("/", HTTP_GET, []() {
    String html = "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>Tank Node</title></head><body style='font-family:system-ui;padding:16px'>";
    html += "<h2>Tank Node</h2>";
    html += "<p>ID: " + String(TANK_ID) + "</p>";
    html += "<p>AP IP: " + apIp.toString() + "</p>";
    html += "<p>Uptime: " + String(millis() / 1000) + "s</p>";
    html += "<p>Last level: " + String("--") + "</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/status", HTTP_GET, []() {
    String json = "{";
    json += "\"id\":" + String(TANK_ID) + ",";
    json += "\"uptimeS\":" + String(millis() / 1000) + ",";
    json += "\"ip\":\"" + apIp.toString() + "\"";
    json += "}";
    server.send(200, "application/json", json);
  });

  server.begin();
}

void loop(){
  static uint32_t lastSendMs = 0;
  const uint32_t now = millis();

  server.handleClient();

  if (now - lastSendMs >= SEND_PERIOD_MS) {
    lastSendMs = now;

    const float gaugeAdcV = readAdcV(PIN_GAUGE_ADC);
    const float gaugeRealV = gaugeAdcV * GAUGE_DIV_RATIO;
    const float levelPct = levelPctFromGaugeV(gaugeRealV);

    const float battAdcV = readAdcV(PIN_BATT_ADC);
    const float battV = battAdcV * BATT_DIV_RATIO;
    const uint8_t battPct = battPctFromV(battV);

    TankPacket p{};
    p.tankId = (uint8_t)TANK_ID;
    p.uptimeS = millis() / 1000;
    p.levelPct_x100 = (uint16_t)roundf(levelPct * 100.0f);
    p.battV_mV = (uint16_t)roundf(battV * 1000.0f);
    p.battPct = battPct;
    p.crc = 0;
    p.crc = checksum16(reinterpret_cast<const uint8_t*>(&p), sizeof(p) - sizeof(p.crc));

    LoRa.beginPacket();
    LoRa.write((const uint8_t*)LORA_KEY, strlen(LORA_KEY));
    LoRa.write(0);
    LoRa.write((uint8_t*)&p, sizeof(p));
    const int ok = LoRa.endPacket();
    if (ok == 1) {
      blinkLed(PIN_LED_COMMS, 40);
    }

    drawStatus(levelPct, battV, battPct);

    Serial.printf("Sent tank=%u level=%.1f%% (gauge=%.2fV) batt=%.2fV (%u%%)\n", p.tankId, levelPct, gaugeRealV, battV, battPct);
  }
}
