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
  // Test with LED blinking - no Serial
  pinMode(2, OUTPUT);  // GPIO 2 - LED pin
  digitalWrite(2, HIGH);
  delay(500);
  digitalWrite(2, LOW);
  
  Serial.begin(115200);
  delay(200);
  Serial.println("\nC3 BOOTED!");
}

void loop(){
  digitalWrite(2, HIGH);
  Serial.println("ON");
  delay(500);
  digitalWrite(2, LOW);
  Serial.println("OFF");
  delay(500);
}
