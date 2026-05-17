#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

#include "config.h"

struct __attribute__((packed)) TankPacket {
  uint8_t tankId;
  uint32_t uptimeS;
  uint16_t levelPct_x100; // 0..10000
  uint16_t battV_mV;      // e.g. 3700
  uint8_t battPct;        // 0..100
  uint16_t crc;           // simple checksum
};

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

void setup(){
  Serial.begin(115200);
  delay(200);

  analogReadResolution(12);

  pinMode(PIN_LED_POWER, OUTPUT);
  pinMode(PIN_LED_COMMS, OUTPUT);
  ledWrite(PIN_LED_COMMS, false);
  ledWrite(PIN_LED_POWER, true);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ_HZ)) {
    Serial.println("LoRa init failed");
    while (true) delay(1000);
  }

  LoRa.setTxPower(17);
  Serial.println("Tank node started");
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

  LoRa.beginPacket();
  LoRa.write((const uint8_t*)LORA_KEY, strlen(LORA_KEY));
  LoRa.write(0); // key terminator
  LoRa.write((uint8_t*)&p, sizeof(p));
  const int ok = LoRa.endPacket();
  if (ok == 1) {
    blinkLed(PIN_LED_COMMS, 40);
  }

  Serial.printf("Sent tank=%u level=%.1f%% (gauge=%.2fV) batt=%.2fV (%u%%)\n", p.tankId, levelPct, gaugeRealV, battV, battPct);
  delay(SEND_PERIOD_MS);
}
