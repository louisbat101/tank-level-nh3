# Tank Level Monitoring System - Status Report

**Date**: May 20, 2026  
**Status**: ✅ BOARDS OPERATIONAL (LoRa TX Issue Pending)

## Hardware Setup ✅

| Device | Port | Board | Status |
|--------|------|-------|--------|
| **Gateway** | `/dev/cu.usbserial-0001` | Heltec WiFi LoRa 32 V3 | ✅ Booting |
| **Tank 1** | `/dev/cu.usbserial-4` | Heltec WiFi LoRa 32 V3 | ✅ Booting |
| **Tank 2** | `/dev/cu.usbserial-3` | Heltec WiFi LoRa 32 V3 | ✅ Booting |

## Firmware Features ✅

### Gateway (Port 0001)
- ✅ WiFi AP: `TankMonitor` at `192.168.4.1`
- ✅ LittleFS web dashboard loaded
- ✅ LoRa RX initialized (915 MHz)
- ✅ Web server on HTTP port 80
- 🟡 RX not receiving packets (LoRa TX issue on Tank nodes)

### Tank Nodes (Ports 3 & 4)
- ✅ WiFi AP: `TankNode-{ID}` with status page
- ✅ ADC reading tank gauge and battery voltage
- ✅ LoRa TX module initialized (915 MHz)
- ✅ Building TankPacket with CRC
- 🔴 **LoRa TX failing with error -5 (RADIOLIB_ERR_TX_TIMEOUT)**

## Key Breakthrough

Fixed critical GPIO pin configuration issue:
- **Problem**: GPIO 36/37 (RST and GPIO0) don't work as outputs on ESP32-S3
- **Solution**: Use GPIO -1 to disable hardware control
- **Result**: All three boards now boot and initialize LoRa successfully

Changed Module initialization from:
```cpp
Module(8, 35, 37, 36, *loRaSPI)  // HANGS - GPIO 37 & 36 unavailable
```

To:
```cpp
Module(8, 35, -1, -1, *loRaSPI)   // WORKS - Disables hardware RST/GPIO0
```

## LoRa Configuration (All Boards)

```
Frequency:  915 MHz
Bandwidth:  125 kHz
SF (Spreading Factor): 7
Coding Rate: 5
Output Power: 17 dBm
Preamble Length: 8
Header Mode: Implicit (packet size: 12 bytes)
```

## Current Blocker: LoRa TX Timeout (-5)

### Symptoms
- Both Tank nodes fail to transmit with `radio->transmit()` returning -5
- Error persists across multiple attempts and configurations
- `radio->begin()` succeeds (returns 0)
- Gateway `startReceive()` succeeds (returns 0)

### Attempted Fixes (All Failed)
1. ✅ Fixed GPIO pin config (worked for init, not TX)
2. ✅ Added standby mode before TX (no change)
3. ✅ Tried explicit vs implicit header modes (no change)
4. ✅ Added 3x retry logic (still -5 on all retries)
5. ✅ Increased preamble length and delays (no change)

### Root Cause Analysis
Error -5 is `RADIOLIB_ERR_TX_TIMEOUT` which occurs when:
1. DIO0 (IRQ) pin interrupt doesn't respond (GPIO 35)
2. Radio chip doesn't properly control TX sequence
3. SPI communication issue during TX (but init works fine)

**Most Likely**: Hardware issue with SX1262 DIO0 pin or TX amplifier

## Files Modified

### Core Firmware
- `firmware/node-heltec/src/main.cpp` - Tank Node implementation
- `firmware/gateway-heltec/src/main.cpp` - Gateway implementation  
- `firmware/node-heltec/include/config.h` - ADC pins (12, 13) + LoRa config

### Key Changes
- RadioLib 6.6.0 library (supports SX1262)
- Module creation: `Module(8, 35, -1, -1, *loRaSPI)`
- ADC pins: GPIO 12 (gauge), GPIO 13 (battery)
- SPI pins: SCK=9, MISO=11, MOSI=10, CS=8

## Next Steps

### Immediate (if continuing work)
1. **Measure DIO0 voltage** on SX1262 to confirm pin is active
2. **Check SPI communication** with logic analyzer during TX attempt
3. **Try different RadioLib version** or switch to alternative LoRa library
4. **Inspect PCB** for solder bridges or component damage near SX1262

### Workarounds
1. Use WiFi instead of LoRa (requires WiFi network setup)
2. Use different LoRa boards (e.g., with SX1272 + SandeepMistry library)
3. Implement wired communication (USB/serial mesh)

### Rollback Options
- Earlier working code using SandeepMistry LoRa library for SX1272/SX1276 boards
- ESP-NOW protocol (had callback trigger issues - see git history)

## Testing Notes

All three boards boot and initialize successfully. To test:
```bash
# Monitor all three boards
cd firmware/node-heltec
timeout 15 python3 << 'EOF'
import serial, time, threading

ports = {
    '/dev/cu.usbserial-0001': 'GATEWAY',
    '/dev/cu.usbserial-3': 'TANK 2',
    '/dev/cu.usbserial-4': 'TANK 1'
}

def read_port(port, name):
    ser = serial.Serial(port, 115200, timeout=0.2)
    ser.dtr = False
    time.sleep(0.1)
    ser.dtr = True
    time.sleep(2)
    
    print(f"\n{'='*50}\n{name}\n{'='*50}")
    for _ in range(100):
        line = ser.readline()
        if line:
            decoded = line.decode('utf-8', errors='replace').rstrip()
            if any(x in decoded for x in ['LoRa', 'TX', 'RX', 'ready']):
                print(f"[{name:8}] {decoded}")
    ser.close()

threads = [threading.Thread(target=read_port, args=(p, n), daemon=True) 
           for p, n in ports.items()]
for t in threads: t.start()
time.sleep(13)
EOF
```

## Build Instructions

```bash
# Build Gateway
cd firmware/gateway-heltec
pio run
pio run -t upload --upload-port /dev/cu.usbserial-0001

# Build Tank Node
cd firmware/node-heltec  
pio run
pio run -t upload --upload-port /dev/cu.usbserial-4  # Tank 1
pio run -t upload --upload-port /dev/cu.usbserial-3  # Tank 2
```

## Dashboard Access

Gateway web dashboard: `http://192.168.4.1`
- Lists connected tank nodes
- Shows online/offline status
- Displays last update time
- Tank node status pages at `TankNode-{ID}.local`

---

**Generated**: May 20, 2026  
**System**: macOS (zsh)  
**IDEs**: VS Code with PlatformIO
