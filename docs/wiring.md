# Wiring notes (placeholder)

## Rochester gauge sensor

Your label shows Rochester Gauges (model family like RD20…). The *electrical interface* depends on the exact sender/transmitter.

Common cases:
- **Resistive sender** (varies with level) used with a gauge.
- **Transmitter** (0–5V, 0–10V, or 4–20mA).

### Important
Do **not** connect unknown sensor outputs directly to ESP32 ADC.

## Your case: 0–5V transmitter

ESP32 ADC pins are **max 3.3V**.

So your 0–5V Rochester signal must go through a **voltage divider** so that:

- 5.0V at the sensor becomes **≤ 3.3V** at the ESP32 ADC

Example divider:

- Rtop = 10k, Rbottom = 20k → 5V * (20k / (10k+20k)) = **3.33V**

If you use a different divider, update `GAUGE_DIV_RATIO` in `firmware/node-esp32c3/include/config.h`.

## ESP32-C3 LEDs

The ESP32-C3 node firmware supports 2 LEDs:

- `PIN_LED_POWER`: on solid when firmware is running
- `PIN_LED_COMMS`: blinks on each successful LoRa send

Pin defaults are placeholders; adjust in `firmware/node-esp32c3/include/config.h`.

## Battery measurement

Typically:
- Li-ion 1S: voltage divider to ADC pin
- Or a fuel gauge (MAX17048 etc.)

## LoRa

- Heltec boards have built-in SX127x; use the board's LoRa pin mapping.
- ESP32-C3 needs an external LoRa module (SX1276/78) wired via SPI.
