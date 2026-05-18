#pragma once

#ifndef LORA_FREQ_HZ
#define LORA_FREQ_HZ 915E6
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
#define OLED_SDA 21
#endif
#ifndef OLED_SCL
#define OLED_SCL 22
#endif
#ifndef OLED_RST
#define OLED_RST -1
#endif
// Heltec boards often require Vext to be enabled (active LOW) to power the OLED.
#ifndef OLED_VEXT_PIN
#define OLED_VEXT_PIN 36
#endif
#ifndef OLED_I2C_ADDR
#define OLED_I2C_ADDR 0x3C
#endif
