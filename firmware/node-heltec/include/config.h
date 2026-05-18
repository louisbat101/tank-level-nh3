#pragma once

#ifndef TANK_ID
#define TANK_ID 2
#endif

// Heltec WiFi LoRa 32 typically uses built-in LoRa pins; LoRa library handles defaults.
#ifndef LORA_FREQ_HZ
#define LORA_FREQ_HZ 915E6
#endif

#ifndef LORA_KEY
#define LORA_KEY ""
#endif

// Rochester 0..5V transmitter MUST be scaled to 0..3.3V ADC with a voltage divider.
// Placeholder ADC pins (update based on your wiring and Heltec ADC availability)
#ifndef PIN_GAUGE_ADC
#define PIN_GAUGE_ADC 36
#endif
#ifndef PIN_BATT_ADC
#define PIN_BATT_ADC 37
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
