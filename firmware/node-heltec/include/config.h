#pragma once

#ifndef TANK_ID
#define TANK_ID 2
#endif

// Heltec WiFi LoRa 32 V3 LoRa configuration
#ifndef LORA_FREQ_HZ
#define LORA_FREQ_HZ 915E6
#endif
// Explicit SPI pins for Heltec V3 (standard ESP32 SPI2)
#ifndef LORA_SCK
#define LORA_SCK 9
#endif
#ifndef LORA_MISO
#define LORA_MISO 11
#endif
#ifndef LORA_MOSI
#define LORA_MOSI 10
#endif
#ifndef LORA_SS
#define LORA_SS 8
#endif
// LoRa pins for SX1262 on Heltec V3
#ifndef LORA_DIO1
#define LORA_DIO1 14  // DIO1 interrupt pin
#endif
#ifndef LORA_RST
#define LORA_RST 12   // Reset pin
#endif
#ifndef LORA_BUSY
#define LORA_BUSY 13  // BUSY signal (CRITICAL!)
#endif

#ifndef LORA_KEY
#define LORA_KEY ""
#endif

// ADC pins - IMPORTANT: GPIO 13 is LoRa BUSY, can't use for ADC!
// Use GPIO 4 and 5 instead (or other available ADC pins)
#ifndef PIN_GAUGE_ADC
#define PIN_GAUGE_ADC 4   // Tank level gauge (was 12, moved to avoid conflicts)
#endif
#ifndef PIN_BATT_ADC
#define PIN_BATT_ADC 5    // Battery voltage (was 13, CONFLICTS with LORA_BUSY!)
#endif

#ifndef ADC_REF_V
#define ADC_REF_V 3.3f
#endif
#ifndef ADC_MAX
#define ADC_MAX 4095.0f
#endif

#ifndef BATT_DIV_RATIO
#define BATT_DIV_RATIO 2.0f
#endif

// Gauge divider ratio: Vgauge(real) = Vadc * GAUGE_DIV_RATIO
// Example: 10k(top) + 20k(bottom) divider => ratio 1.5
#ifndef GAUGE_DIV_RATIO
#define GAUGE_DIV_RATIO 1.5f
#endif

// 0..5V transmitter scaling (REAL sensor volts)
#ifndef GAUGE_V_EMPTY
#define GAUGE_V_EMPTY 0.0f
#endif
#ifndef GAUGE_V_FULL
#define GAUGE_V_FULL 5.0f
#endif

#ifndef SEND_PERIOD_MS
#define SEND_PERIOD_MS 5000
#endif

// ====== Tank node WiFi AP (status page) ======
#ifndef NODE_AP_SSID_PREFIX
#define NODE_AP_SSID_PREFIX "TankNode-"
#endif
#ifndef NODE_AP_PASS
#define NODE_AP_PASS "tanknode"
#endif

// ====== LEDs (Heltec) ======
// Uses the board's built-in LED by default.
#ifndef PIN_LED_POWER
#define PIN_LED_POWER LED_BUILTIN
#endif
#ifndef PIN_LED_COMMS
#define PIN_LED_COMMS LED_BUILTIN
#endif
#ifndef OLED_RST
// Heltec V3 OLED reset is often not connected; keep -1 unless you know the pin.
#define OLED_RST -1
#endif
// Vext powers the OLED on many Heltec boards (LOW = ON).
#ifndef OLED_VEXT_PIN
#define OLED_VEXT_PIN 21
#endif

// ====== OLED (Heltec V3) ======
// Default pins may need adjustment depending on board revision.
#ifndef OLED_SDA
#define OLED_SDA 17
#endif
#ifndef OLED_SCL
#define OLED_SCL 18
#endif
// Use -1 if OLED reset is not connected.
#ifndef OLED_RST
#define OLED_RST -1
#endif
// Heltec boards often require Vext to be enabled (active LOW) to power the OLED.
// Set to your board's Vext pin (commonly GPIO21); -1 disables.
#ifndef OLED_VEXT_PIN
#define OLED_VEXT_PIN 21
#endif
// I2C address (0x3C is most common)
#ifndef OLED_I2C_ADDR
#define OLED_I2C_ADDR 0x3C
#endif
