# Requirements

## Toolchain

[PlatformIO](https://platformio.org) — VS Code extension **PlatformIO IDE**, or PlatformIO Core (`pio`) 6.2.0+.

Everything else is declared in `platformio.ini` and installed automatically on the first build.

---

## Platform

| Component | Version |
|---|---|
| Platform | [pioarduino/platform-espressif32](https://github.com/pioarduino/platform-espressif32) **55.03.313** |
| Arduino-ESP32 core | **3.3.13** |
| Partition scheme | `min_spiffs.csv` — Minimal SPIFFS (1.9 MB app with OTA) |

The official `platformio/espressif32` platform only ships Arduino-ESP32 core 2.x; this firmware needs core 3.x, hence pioarduino.

---

## Libraries

| Library | Version | Purpose |
|---|---|---|
| m5stack/M5Unified | 0.3.0 | display, buttons, IMU, battery — both boards |
| m5stack/M5GFX | 0.3.0 | graphics (used by M5Unified) |
| links2004/WebSockets | 2.7.2 | TallyArbiter connection |
| bblanchon/ArduinoJson | 7.4.3 | TallyArbiter messages |
| tzapu/WiFiManager | 2.0.17 | setup AP, web config, firmware update page |

---

## OTA Updates

Via the built-in web portal:

1. Find the device IP on the Settings screen (M5 button single click)
2. Open that address in any browser on the same network
3. Go to **Update** in the portal menu and upload `release/GP_tally_M5StickC_v<ver>.bin` (not the `_full` file)

Via PlatformIO:

```sh
pio run -e m5stickc-ota -t upload --upload-port <device-ip>
```

- Hostname: `m5StickC-xxxxxx` / `m5StickC-plus-xxxxxx`
- Password: `tallyarbiter`
