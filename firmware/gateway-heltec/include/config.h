#pragma once

// ====== LoRa (SX1262 on Heltec WiFi LoRa 32 V3) ======
#ifndef LORA_FREQ_HZ
#define LORA_FREQ_HZ 915E6
#endif
// Heltec V3 SPI pins for LoRa (standard ESP32 SPI2)
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
// Note: RST and DIO0 may not be needed for LoRa library
#ifndef LORA_RST
#define LORA_RST -1
#endif
#ifndef LORA_DIO0
#define LORA_DIO0 -1
#endif

// Default WiFi AP (gateway always starts AP so a tablet can connect)
#ifndef AP_SSID
#define AP_SSID "TankMonitor"
#endif
#ifndef AP_PASS
#define AP_PASS "tankmonitor"
#endif

// How often UI clients should get full state over WS
#ifndef WS_STATE_PERIOD_MS
#define WS_STATE_PERIOD_MS 1000
#endif

// Default offline timeout (seconds) - can be overridden in config via UI
#ifndef DEFAULT_OFFLINE_TIMEOUT_S
#define DEFAULT_OFFLINE_TIMEOUT_S 30
#endif

// ====== OLED (Heltec V3) ======
#ifndef USE_OLED
#define USE_OLED 0
#endif
#ifndef OLED_SDA
#define OLED_SDA 17
#endif
#ifndef OLED_SCL
#define OLED_SCL 18
#endif
#ifndef OLED_RST
#define OLED_RST -1
#endif
// Heltec boards often require Vext to be enabled (active LOW) to power the OLED.
#ifndef OLED_VEXT_PIN
#define OLED_VEXT_PIN 21
#endif
#ifndef OLED_I2C_ADDR
#define OLED_I2C_ADDR 0x3C
#endif
