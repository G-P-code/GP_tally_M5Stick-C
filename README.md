# GP Tally M5StickC

An Arduino firmware for the [M5StickC, M5StickC-Plus, and M5StickC-Plus2](https://m5stack.com) that turns the device into a wireless tally light listener client for [TallyArbiter](https://github.com/josephdadams/TallyArbiter).

Based on the original [m5stickc-listener](https://github.com/josephdadams/TallyArbiter/tree/master/listener_clients/m5stickc-listener) by Joseph Adams.

---

## Features

- Connects to a TallyArbiter server over Wi-Fi via Socket.IO
- Displays tally state (Program / Preview / Aux) as a full-screen background colour with device name
- Optional external LED outputs for Program, Preview, and Aux signals with PWM brightness control
- Automatic screen rotation via IMU
- Captive portal configuration (Wi-Fi credentials + TallyArbiter server address) via WifiManager
- Settings screen with live connection status, IP address, battery level, and firmware version
- Battery status: **Charging...** / **Charged.** / **nn%** — auto-updates while Settings screen is open
- Adjustable display and LED brightness
- LED toggle (enable/disable external LEDs at runtime)
- LED sequential check animation for wiring verification
- Flash and reassign animations triggered by TallyArbiter server events
- Over-the-air (OTA) firmware updates
- Exponential backoff reconnection
- Adaptive Wi-Fi TX power and CPU frequency for battery efficiency

---

## Supported Hardware

| Board | Arduino IDE Board |
|---|---|
| M5StickC | `M5Stack Arduino > M5StickC` |
| M5StickC-Plus | `M5Stack Arduino > M5StickCPlus` |
| M5StickC-Plus2 | `M5Stack Arduino > M5StickCPlus2` |

Board selection is automatic when using the Arduino IDE board selector. Manual override defines are available in `GP_tally_M5Stick-C.ino` if needed.

---

## Requirements

### Libraries

Install via Arduino Library Manager:

- [M5StickC](https://github.com/m5stack/M5StickC) / [M5StickC-Plus](https://github.com/m5stack/M5StickC-Plus) / [M5StickC-Plus2](https://github.com/m5stack/M5StickCPlus2)
- [SocketIoClient](https://github.com/timum-viw/socket.io-client)
- [WebSockets](https://github.com/Links2004/arduinoWebSockets) — **version 2.3.4**
- [Arduino_JSON](https://github.com/arduino-libraries/Arduino_JSON)
- [WifiManager](https://github.com/tzapu/WiFiManager) by tzapu
- [MultiButton](https://github.com/poelstra/arduino-multi-button)

---

## Installation

1. Clone or download this repository.
2. Open `GP_tally_M5Stick-C/GP_tally_M5Stick-C.ino` in Arduino IDE.
3. Select the correct board under **Tools > Board**.
4. Set **Tools > Partition Scheme** to **Minimal SPIFFS (... MB App with OTA / ... SPIFFS)**. This is required to fit the firmware with OTA support.
5. Install all required libraries.
6. Flash to your device.

---

## First-time Setup

1. Power on the device. It will create a Wi-Fi access point named **m5StickC-plus-xxxxxx**.
2. Connect to that network from your phone or computer (no password required).
3. Open a browser and go to **192.168.6.1**.
4. Enter your Wi-Fi credentials and the TallyArbiter server IP and port (default: `4455`).
5. Click Save. The device restarts and connects automatically.

---

## Controls

| Control | Action |
|---|---|
| **M5 button** — single click | Toggle between Tally screen and Settings screen |
| **M5 button** — double click | Toggle external LEDs on / off |
| **M5 button** — hold 5 s | Reset Wi-Fi settings and restart |
| **Action button** — single click | Cycle display brightness |
| **Action button** — long press | Run LED sequential check |
| **Grove pin G33** — short press | Open configuration portal |
| **Grove pin G33** — hold 3 s | Erase all settings and restart |

---

## External LED Wiring (optional)

| Signal | Pin | Pin (Plus2) |
|---|---|---|
| Program | G0 | G19 |
| Preview | G26 | G26 |
| Aux | G25 | G25 |

LED brightness tracks the display brightness setting. LEDs can be toggled on/off at runtime with a double click on the M5 button without affecting the tally display.

---

## Configuration & OTA Updates

### Web portal

While the Settings screen is open on the device, a configuration web portal is active and accessible from any browser on the same network.

Navigate to the device IP address shown on the Settings screen.

The portal allows you to:
- Change the Wi-Fi network
- Change the TallyArbiter server address and port
- Update the firmware (OTA) without Arduino IDE — go to **Update** in the portal menu and upload a compiled `.bin` file

The device restarts automatically after saving any changes.

### OTA via Arduino IDE

Firmware can also be updated over the air directly from Arduino IDE (**Sketch > Upload**  — select the device under **Tools > Port > Network ports**).

- Hostname: `m5StickC-plus-xxxxxx`
- Password: `tallyarbiter`

---

## Project Structure

```
GP_tally_M5Stick-C/
├── GP_tally_M5Stick-C.ino  — board detection, globals, setup(), loop()
├── config.h                 — all #define constants and NVS preference keys
├── screen.ino               — screen state management and painting
├── buttons.ino              — brightness, LED check, reset button
├── tally.ino                — Socket.IO events and tally data processing
├── network.ino              — Wi-Fi, WifiManager, reconnection
├── leds.ino                 — external LED control and animations
├── power.ino                — CPU performance mode and battery cache
└── m5hal.ino                — M5Stack hardware abstraction layer
```

---

## License

MIT
