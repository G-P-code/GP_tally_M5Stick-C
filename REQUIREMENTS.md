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

| Library | Version | Used by |
|---|---|---|
| links2004/WebSockets | 2.7.2 | all |
| bblanchon/ArduinoJson | 7.4.3 | all |
| tzapu/WiFiManager | 2.0.17 | all |
| poelstra/MultiButton | 1.3.0 | all |
| m5stack/M5StickC | 0.3.0 (from GitHub) | `m5stickc` |
| m5stack/M5StickCPlus | 0.1.1 | `m5stickc-plus` |
| m5stack/M5StickCPlus2 | 1.0.2 (+ M5Unified / M5GFX) | `m5stickc-plus2` |

Each environment pulls in only its own M5Stack library, so the `multiple definition of 'M5'` conflict from the Arduino IDE cannot happen.

M5StickC 0.3.0 is not published on the PlatformIO registry (the registry stops at 0.2.5, which does not compile on core 3.x), so it is fetched from the GitHub tag.

---

## OTA Updates

Via the built-in web portal:

1. Open the Settings screen on the device (M5 button single click)
2. Navigate to the device IP address shown on screen from any browser on the same network
3. Go to **Update** in the portal menu and upload `.pio/build/<env>/firmware.bin`

Via PlatformIO:

```sh
pio run -e m5stickc-plus2-ota -t upload --upload-port <device-ip>
```

- Hostname: `m5StickC-plus-xxxxxx`
- Password: `tallyarbiter`
