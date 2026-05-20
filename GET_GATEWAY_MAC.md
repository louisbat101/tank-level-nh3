# Finding the Gateway MAC Address for ESP-NOW

## Quick Method

1. **Disconnect the C3 node** from USB
2. **Connect the Heltec Gateway** (ESP32-S3) via USB
3. **Open the serial monitor** in PlatformIO:
   - In VS Code, with the gateway project open, press `Ctrl+Shift+A` → Terminal → New Terminal
   - Run: `pio device monitor -b 115200`
4. **Look for lines like this on startup:**
   ```
   Gateway MAC (for ESP-NOW): AC:A7:04:BD:FC:CC
   Gateway MAC hex: {0xAC, 0xA7, 0x04, 0xBD, 0xFC, 0xCC}
   ```

## Update the C3 Config

Once you have the gateway's MAC, edit `firmware/node-esp32c3/include/config.h`:

```cpp
#ifndef GATEWAY_MAC
#define GATEWAY_MAC {0xAC, 0xA7, 0x04, 0xBD, 0xFC, 0xCC}  // Replace with your gateway MAC
#endif
```

Then rebuild and upload the C3 firmware.

## Alternative (without serial monitor)

If you don't have the gateway handy, you can:
1. Look at the gateway's OLED display (if available) - it may show the MAC
2. Check the gateway's web dashboard at `http://192.168.4.1` (if configured)
3. Use a network scanner tool to find ESP devices on the network
