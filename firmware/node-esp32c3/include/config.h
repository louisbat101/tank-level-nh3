#pragma once

// ====== Node identity ======
// Set unique tank ID per device (1..8)
#ifndef TANK_ID
#define TANK_ID 1
#endif

// ====== LoRa (external SX127x module on ESP32-C3) ======
// Placeholder pins - MUST be updated for your wiring.
// SPI pins may also be set by your board variant.
#ifndef LORA_SCK
#define LORA_SCK 4
#endif
#ifndef LORA_MISO
#define LORA_MISO 5
#endif
#ifndef LORA_MOSI
#define LORA_MOSI 6
#endif
#ifndef LORA_SS
#define LORA_SS 7
#endif
#ifndef LORA_RST
#define LORA_RST 8
#endif
#ifndef LORA_DIO0
#define LORA_DIO0 9
#endif

// Set your region frequency (US=915E6, EU=868E6, etc.)
#ifndef LORA_FREQ_HZ
#define LORA_FREQ_HZ 915E6
#endif

// Shared key string (simple token, not encryption)
#ifndef LORA_KEY
#define LORA_KEY ""
#endif

// ====== ADC placeholders ======
// Rochester gauge input (0..5V transmitter MUST be scaled to 0..3.3V ADC)
#ifndef PIN_GAUGE_ADC
#define PIN_GAUGE_ADC 0
#endif
// Battery divider (0..3.3V)
#ifndef PIN_BATT_ADC
#define PIN_BATT_ADC 1
#endif

// ADC calibration placeholders
// Convert raw ADC reading into voltage at the ADC pin.
// You might want to use analogReadMilliVolts() if available.
#ifndef ADC_REF_V
#define ADC_REF_V 3.3f
#endif
#ifndef ADC_MAX
#define ADC_MAX 4095.0f
#endif

// Battery divider ratio: Vbatt = Vadc * BATT_DIV_RATIO
#ifndef BATT_DIV_RATIO
#define BATT_DIV_RATIO 2.0f
#endif

// Gauge divider ratio: Vgauge(real) = Vadc * GAUGE_DIV_RATIO
// Example: if you use 10k(top) + 20k(bottom) divider (5V -> 3.33V): ratio ~ 3.0/2.0? actually (Rtop+Rbot)/Rbot.
// For 10k + 20k: ratio = (10k+20k)/20k = 1.5
#ifndef GAUGE_DIV_RATIO
#define GAUGE_DIV_RATIO 1.5f
#endif

// Gauge scaling for a 0..5V transmitter (in REAL sensor volts)
#ifndef GAUGE_V_EMPTY
#define GAUGE_V_EMPTY 0.0f
#endif
#ifndef GAUGE_V_FULL
#define GAUGE_V_FULL 5.0f
#endif

// ====== LEDs (ESP32-C3) ======
// Power/status LED: on solid when running
#ifndef PIN_LED_POWER
#define PIN_LED_POWER 2
#endif
// Comms LED: blinks briefly each successful LoRa send
#ifndef PIN_LED_COMMS
#define PIN_LED_COMMS 3
#endif
#ifndef LED_ACTIVE_HIGH
#define LED_ACTIVE_HIGH 1
#endif

// Send period
#ifndef SEND_PERIOD_MS
#define SEND_PERIOD_MS 5000
#endif
