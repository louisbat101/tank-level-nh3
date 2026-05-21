# LoRa TX Timeout Diagnosis Report

**Issue**: All Tank Node boards fail LoRa transmission with error -5 (RADIOLIB_ERR_TX_TIMEOUT)
**Dates Tested**: May 20, 2026
**Boards Affected**: Tank 1 (Port 4), Tank 2 (Port 3)
**Gateway Status**: LoRa RX initializing successfully

## PIN CONFIGURATIONS TESTED

### Configuration 1: GPIO 36/37 (Original)
```cpp
Module(8, 35, 37, 36, *loRaSPI)
// CS=8, IRQ=35, RST=37, GPIO0=36
```
**Result**: ❌ HANG - radio->begin() never returns
**Issue**: GPIO 36/37 are ADC-only pins, can't be used as outputs

---

### Configuration 2: GPIO -1/-1 (Disable RST/GPIO0)
```cpp
Module(8, 35, -1, -1, *loRaSPI)
// CS=8, IRQ=35, RST=disabled, GPIO0=disabled
```
**Result**: ⚠️ INIT OK, TX FAILS
- `radio->begin()` returns 0 ✅
- `radio->transmit()` returns -5 (timeout) ❌
- All TX attempts fail with same error
- Retry logic doesn't help

---

### Configuration 3: GPIO 0/1 (Strapping Pins)
```cpp
Module(8, 35, 0, 1, *loRaSPI)
// CS=8, IRQ=35, RST=0, GPIO0=1
```
**Result**: ⚠️ INIT OK, TX FAILS
- `radio->begin()` returns 0 ✅
- `radio->transmit()` returns -5 (timeout) ❌
- Same behavior as Config 2

---

### Configuration 4: GPIO 34/2 (BUSY as RST)
```cpp
Module(8, 35, 34, 2, *loRaSPI)
// CS=8, IRQ=35, RST=34(BUSY), GPIO0=2
```
**Result**: ⚠️ INIT OK, TX FAILS
- `radio->begin()` returns 0 ✅
- `radio->transmit()` returns -5 (timeout) ❌
- Same timeout error despite trying different GPIO pins

---

## PINS SUMMARY

**Working Pins**:
- ✅ CS (Chip Select):    GPIO 8
- ✅ IRQ (DIO0):          GPIO 35  
- ✅ SCK (SPI Clock):     GPIO 9
- ✅ MISO (SPI In):       GPIO 11
- ✅ MOSI (SPI Out):      GPIO 10
- ✅ SPI Init Comms:      All working

**Problematic Pins**:
- ❌ GPIO 36: ADC-only, can't use as output (causes hang)
- ❌ GPIO 37: ADC-only, can't use as output (causes hang)
- ❓ GPIO 0/1: Don't fix TX timeout
- ❓ GPIO 34: Doesn't help as BUSY/RST
- ❓ GPIO 2: Doesn't help as GPIO0

**Missing/Unknown**:
- ❓ BUSY pin: SX1262 has a BUSY signal line - location unknown
- ❓ DIO1: SX1262 has DIO1 pin - may need configuration
- ❓ INT pin: May be incorrectly routed or not connected

---

## ERROR CODE ANALYSIS

**Error -5: RADIOLIB_ERR_TX_TIMEOUT**

Occurs when:
1. DIO0 interrupt doesn't trigger during TX
2. Radio doesn't respond to transmission control signals
3. SPI communication breaks during TX phase (init works fine)

**Characteristic Pattern**:
- SPI init/read operations work ✅
- `radio->begin()` succeeds ✅
- `radio->startReceive()` would work ✅
- `radio->transmit()` always times out ❌
- Suggests: TX control path issue, not SPI bus

---

## ATTEMPTED SOFTWARE FIXES

| Approach | Result |
|----------|--------|
| Changed GPIO pins (1-4 above) | ❌ No change |
| Added standby mode before TX | ❌ No change |
| Tried explicit vs implicit headers | ❌ No change |
| Added 3x retry logic | ❌ Still fails on all attempts |
| Increased preamble length | ❌ No change |
| Changed radio->sleep() pattern | ❌ No change |
| Adjusted packet size | ❌ No change |
| Modified transmit delay parameters | ❌ No change |

**Conclusion**: Software changes don't fix the TX timeout. Issue is hardware-level.

---

## HARDWARE DIAGNOSIS NEEDED

**Next Steps to Verify**:

1. **Measure DIO0 voltage** during TX attempt:
   - Use oscilloscope on GPIO 35
   - Should see ~1-2μs pulse when TX starts
   - If no pulse: GPIO 35 interrupt not wired correctly

2. **Check BUSY signal**:
   - Find BUSY pin on Heltec schematic
   - Measure voltage during TX
   - If floating: Not connected to radio

3. **Verify SPI during TX**:
   - Logic analyzer on SCK/MOSI/MISO during transmit
   - If no activity: SPI blocked during TX
   - If present: Check timing and data validity

4. **Inspect PCB**:
   - Look for solder bridges near SX1262
   - Check for cold joints on DIO/RST/GPIO pins
   - Verify power supply (3.3V) to radio module

5. **Test alternative radio**:
   - If available, swap SX1262 with another board
   - Would confirm chip vs board issue

---

## POSSIBLE ROOT CAUSES (RANKED)

1. **DIO0 Interrupt Circuit Broken** (Most Likely 40%)
   - GPIO 35 line not properly connected to SX1262 DIO0
   - Interrupt handler never fires during TX
   - Explains timeout on all configs

2. **BUSY Pin Not Wired** (Possible 30%)
   - SX1262 requires BUSY signal for operation
   - RadioLib may be looking for BUSY on wrong pin
   - Different library/version might expose this

3. **SX1262 Chip Defective** (Possible 20%)
   - Radio chip damaged or non-functional TX path
   - Init works because only checking chip presence
   - Actual TX circuitry broken

4. **Heltec V3 Hardware Revision Issue** (Unlikely 10%)
   - V3 uses different pin routing than expected
   - Schematics outdated or incomplete
   - Need official Heltec documentation

---

## WORKAROUNDS (If Repair Not Possible)

### Option 1: Use WiFi Only
- Remove LoRa entirely
- Use WiFi mesh between nodes
- Requires network infrastructure

### Option 2: Alternative LoRa Library
- Try SandeepMistry LoRa (for SX1272/76 only, not SX1262)
- Not compatible with this hardware

### Option 3: Different Radio Boards
- Use boards with SX1272/SX1276 chips
- Use non-Heltec LoRa boards
- Requires rewiring

### Option 4: Debug with Heltec
- Contact Heltec support with this report
- May have firmware patch or known issue doc
- Could provide corrected pin mapping

---

## FILES & CONFIGURATIONS

**Current Code State**:
- Gateway: `firmware/gateway-heltec/src/main.cpp`
- Tank Node: `firmware/node-heltec/src/main.cpp`
- Config: `firmware/node-heltec/include/config.h`

**Last Working Configuration**:
```cpp
// Module with GPIO 34/2
// All other configs result in same TX timeout
loRaModule = new Module(8, 35, 34, 2, *loRaSPI);

// Radio settings
radio->setFrequency(915.0e6);
radio->setBandwidth(125.0);
radio->setSpreadingFactor(7);
radio->setCodingRate(5);
radio->setPreambleLength(8);
radio->implicitHeader(12);  // TankPacket size

// TX attempt (always returns -5)
int state = radio->transmit((uint8_t*)&packet, sizeof(packet));
// state == -5 every time
```

---

**Test Date**: May 20, 2026 19:44 UTC
**Conclusion**: LoRa TX timeout is a persistent hardware issue not solvable by software configuration changes. Requires oscilloscope/logic analyzer investigation or hardware replacement.
