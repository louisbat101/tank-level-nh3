# LoRa RX Debugging Status

## Current Situation
- ✅ All 3 boards boot successfully
- ✅ Tank 1 and Tank 2 transmit LoRa packets ("TX success" confirmed)
- ✅ Gateway enters RX mode (`startReceive()` returns 0)
- ✅ Gateway loop is running and checking `radio->available()`
- ❌ **Gateway NOT detecting incoming packets** - `radio->available()` always returns 0 or timeout

## ChatGPT Fixes Applied
1. ✅ Added `radio->setCRC(true)` to BOTH sides
2. ✅ Added `radio->setCurrentLimit(140.0)` to BOTH sides
3. ✅ Both sides use `radio->setSyncWord(0x12)`
4. ✅ Both sides use identical frequency/BW/SF/CR settings
5. ✅ Removed `explicitHeader()` / `implicitHeader()` calls - let RadioLib decide
6. ✅ Tank nodes use `radio->standby()` instead of `radio->sleep()` after TX
7. ✅ Gateway uses `radio->startReceive()` in setup

## Configuration (Both TX and RX)
```cpp
radio->setSpreadingFactor(7);
radio->setBandwidth(125.0);
radio->setCodingRate(5);
radio->setSyncWord(0x12);
radio->setCurrentLimit(140.0);
radio->setPreambleLength(8);
radio->setCRC(true);
// NO explicit/implicit header setup
```

## Testing Log

### Test 1: Basic RX Test
- Tank 1 transmitting every ~5 seconds
- Gateway in RX mode
- Result: NO packets detected by `radio->available()`

### Test 2: With Explicit Header
- Changed Gateway to `radio->explicitHeader()`
- Changed Tank to `radio->explicitHeader()`
- Result: Still no packets

### Test 3: With Different Implicit Header Sizes
- Tried implicit header with 12 bytes
- Tried explicit header mode
- Result: Still no packets

## Possible Remaining Issues

### High Priority (Most Likely)
1. **RX mode not actually active** - `startReceive()` might not put radio into continuous RX
2. **DIO1 interrupt not configured** - RadioLib might need interrupt setup
3. **Packet not actually being transmitted** - Tank side might fail silently

### Medium Priority
4. **Frequency drift** - Both sides on slightly different frequencies
5. **TX power too low** - 17 dBm might not be enough for 3 feet
6. **IQ inversion mismatch** - One side inverts IQ, other doesn't

### Low Priority  
7. **CRC mismatch** - Unlikely, both sides set it the same
8. **Library version issue** - Using RadioLib@6.6.0 on both

## Next Steps to Try

1. **Add TX confirmation on Tank side** - Ensure `transmit()` is actually sending RF
   - Add GPIO feedback (LED blink on successful TX)
   - Monitor RSSI or other radio state

2. **Test Gateway with simple echo** - Have Tank transmit, Gateway blink LED on RX
   - Use GPIO to signal packet detection
   - Verify radio is physically receiving

3. **Check RadioLib examples** - Review official RadioLib SX1262 RX example
   - Look for RX interrupt setup
   - Check for proper `startReceive()` usage

4. **Try disabling WiFi/web server** - See if interference from WiFi prevents RX
   - Comment out WiFi initialization
   - Test if LoRa RX works standalone

5. **Monitor radio state** - Print radio mode/status continuously
   - Check if radio switches modes unexpectedly
   - Monitor power consumption (might indicate mode switches)

## File Locations
- Gateway main: `/firmware/gateway-heltec/src/main.cpp`
- Tank main: `/firmware/node-heltec/src/main.cpp`
- Config: `/firmware/node-heltec/include/config.h`

## Hypothesis
Given that TX works but RX doesn't despite all settings being identical, the problem is likely:
- RadioLib's `radio->available()` on SX1262 might have a different behavior than expected
- OR the radio is not actually in RX mode despite `startReceive()` returning success
- OR DIO1 interrupt needs explicit setup for `available()` to work properly

