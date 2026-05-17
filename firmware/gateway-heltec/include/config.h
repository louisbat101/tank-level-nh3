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
