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

// ====== LEDs (Heltec) ======
// Uses the board's built-in LED by default.
#ifndef PIN_LED_POWER
#define PIN_LED_POWER LED_BUILTIN
#endif
#ifndef PIN_LED_COMMS
#define PIN_LED_COMMS LED_BUILTIN
#endif
// Many built-in LEDs are active-low; set to 0 if your LED is inverted.
#ifndef LED_ACTIVE_HIGH
#define LED_ACTIVE_HIGH 0
#endif
