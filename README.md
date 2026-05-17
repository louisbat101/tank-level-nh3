# Tank Level NH3 (ESP32 + LoRa)

This repo contains firmware for **two tank sensor nodes** and a **LoRa gateway** that hosts a local web UI.

- Node 1: **ESP32-C3** (tank sensor + battery + LoRa send)
- Node 2: **Heltec ESP32 LoRa** (tank sensor + battery + LoRa send)
- Gateway: **Heltec ESP32 LoRa** (LoRa receive + WiFi webserver + UI)

## What you get

- Web UI shows **Tank 1** + **Tank 2**:
  - Level (%)
  - Battery (V and % estimate)
  - Online/Offline (green/red)
  - Time-to-empty estimate
- Setup page:
  - select **number of tanks** (1–8)
  - tank name, capacity, daily consumption, offline timeout
  - settings saved on gateway (NVS)

## Repo layout

- `firmware/node-esp32c3` – PlatformIO project for ESP32-C3 node
- `firmware/node-heltec` – PlatformIO project for Heltec node
- `firmware/gateway-heltec` – PlatformIO project for Heltec gateway
- `ui` – UI source files (served by gateway)
- `docs` – wiring + sensor notes

## Build / Flash (PlatformIO)

Open each `firmware/*` folder in PlatformIO and build/upload.

### Gateway UI upload (LittleFS)

The gateway serves the UI from `firmware/gateway-heltec/data/` using **LittleFS**.

In PlatformIO (VS Code):
- run **“Upload File System Image”** for `firmware/gateway-heltec`
- then upload the gateway firmware

> Notes:
> - Pin mappings are **placeholders** in code and must be set to match your wiring.
> - LoRa frequency / pins are configurable in `include/config.h`.

## How it works (high level)

- Each node reads:
  - Rochester gauge input (ADC → level %)
  - battery voltage (ADC → V + % estimate)
- Node sends a compact LoRa packet every few seconds
- Gateway receives packets, updates per-tank state, and serves UI on WiFi

## Next hardware info I need from you (to finalize pin defaults)

1. Which Rochester gauge output type are you using?
   - resistive (ohms)
   - 0–5V / 4–20mA transmitter
2. How are you converting it to an ESP32 ADC input (divider, shunt, module)?
3. Battery measurement method (divider values or a fuel gauge IC?)

Once those are confirmed, I’ll lock in the ADC math and default pins.
