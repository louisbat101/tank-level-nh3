#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

#include "config.h"

struct __attribute__((packed)) TankPacket {
  uint8_t tankId;
  uint32_t uptimeS;
  uint16_t levelPct_x100; // 0..10000
  uint16_t battV_mV;      // e.g. 3700
  uint8_t battPct;        // 0..100
  uint16_t crc;           // simple checksum
};

static uint8_t gatewayMac[6];

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

static float levelPctFromGaugeV(float v){
  float t = (v - GAUGE_V_EMPTY) / (GAUGE_V_FULL - GAUGE_V_EMPTY);
  return clamp01(t) * 100.0f;
}

static uint8_t battPctFromV(float vbatt){
  // Very rough 1S Li-ion estimate. Replace with your battery chemistry curve.
  // 3.2V=0%, 4.2V=100%
  float t = (vbatt - 3.2f) / (4.2f - 3.2f);
  return (uint8_t)roundf(clamp01(t) * 100.0f);
}

static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    blinkLed(PIN_LED_COMMS, 40);
  }
}

void setup(){
  Serial.begin(115200);
  delay(200);
  Serial.println("\n\n=== C3 Tank Node Starting ===");

  // Initialize gateway MAC from config
  uint8_t mac_init[] = GATEWAY_MAC;
  memcpy(gatewayMac, mac_init, 6);
  Serial.printf("Gateway MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
    gatewayMac[0], gatewayMac[1], gatewayMac[2], gatewayMac[3], gatewayMac[4], gatewayMac[5]);

  analogReadResolution(12);

  pinMode(PIN_LED_POWER, OUTPUT);
  pinMode(PIN_LED_COMMS, OUTPUT);
  ledWrite(PIN_LED_COMMS, false);
  ledWrite(PIN_LED_POWER, true);

  // Initialize WiFi in station mode (required for ESP-NOW)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.println("WiFi initialized in STA mode");

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) delay(1000);
  }
  Serial.println("ESP-NOW initialized");

  // Register send callback
  esp_now_register_send_cb(onDataSent);
  Serial.println("Send callback registered");

  // Add gateway peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, gatewayMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
  } else {
    Serial.println("Gateway peer added successfully");
  }

  Serial.println("Tank node started (ESP-NOW mode)");
}

void loop(){
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

  esp_err_t result = esp_now_send(gatewayMac, (uint8_t*)&p, sizeof(p));
  
  Serial.printf("Sent tank=%u level=%.1f%% (gauge=%.2fV) batt=%.2fV (%u%%) [%s]\n", 
    p.tankId, levelPct, gaugeRealV, battV, battPct, 
    (result == ESP_OK) ? "OK" : "FAIL");
  
  delay(SEND_PERIOD_MS);
}
