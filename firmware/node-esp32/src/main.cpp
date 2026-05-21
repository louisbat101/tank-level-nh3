#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

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
  delay(500);
  Serial.println("Tank1 Ready");
  
  uint8_t mac_init[] = GATEWAY_MAC;
  memcpy(gatewayMac, mac_init, 6);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW FAIL");
    while (true) delay(1000);
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, gatewayMac, 6);
  peerInfo.channel = 0;  // Auto
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
  Serial.println("Peer added");
}

void loop(){
  TankPacket p{};
  p.tankId = 1;
  p.uptimeS = millis() / 1000;
  p.levelPct_x100 = 5000;  // 50%
  p.battV_mV = 4200;
  p.battPct = 100;
  p.crc = 0;
  p.crc = checksum16((uint8_t*)&p, sizeof(p) - 2);

  esp_now_send(gatewayMac, (uint8_t*)&p, sizeof(p));
  Serial.println("Sent");
  delay(5000);
}
