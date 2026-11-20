# GP Tally M5StickC

A PlatformIO / Arduino firmware for the [M5StickC and M5StickC-Plus](https://m5stack.com) that turns the device into a wireless tally light listener client for [TallyArbiter](https://github.com/josephdadams/TallyArbiter).

Based on the original [m5stickc-listener](https://github.com/josephdadams/TallyArbiter/tree/master/listener_clients/m5stickc-listener) by Joseph Adams.

---

## Features

- Connects to a TallyArbiter server over Wi-Fi via Socket.IO
- Displays tally state (Program / Preview / Aux) as a full-screen background colour with device name
- Device MAC suffix shown in bottom-right corner of the tally screen for easy identification
- Optional external LED outputs for Program, Preview, and Aux signals with PWM brightness control
- Automatic screen rotation via IMU
- Captive portal configuration (Wi-Fi credentials + TallyArbiter server address) via WifiManager
- Settings screen with live connection status, IP address, battery level, and firmware version
- Battery status: **Charging...** / **Charged.** / **nn%** — auto-updates while Settings screen is open
- Adjustable display and LED brightness with OSD bar
- LED toggle (enable/disable external LEDs at runtime)
- LED sequential check animation for wiring verification
- Flash and reassign animations triggered by TallyArbiter server events
- Over-the-air (OTA) firmware updates
- Exponential backoff reconnection
- Adaptive Wi-Fi TX power and CPU frequency for battery efficiency

---

## Supported Hardware

| Board | Firmware |
|---|---|
| M5StickC | `GP_tally_M5StickC_v<ver>.bin` |
| M5StickC-Plus | same file |

One firmware for both: the board is detected at run time.

---

## Installing a Release

Download from [Releases](https://github.com/G-P-code/GP_tally_M5Stick-C/releases):

| File | Use |
|---|---|
| `GP_tally_M5StickC_v<ver>_full.bin` | New or erased device, flashed over USB at address **0x0** (bootloader + partitions + firmware). **Erases the saved WiFi and TallyArbiter settings.** |
| `GP_tally_M5StickC_v<ver>.bin` | Update of a working device: web portal **Update**, or over USB at address **0x10000**. Settings are kept. |

Over USB without installing anything: open [ESP Tool](https://espressif.github.io/esptool-js/) in Chrome or Edge, **Connect**, add the file at the address above and **Program**. Or from the command line:

```sh
esptool.py --chip esp32 write_flash 0x0 GP_tally_M5StickC_v4.6_full.bin
```

---

## Building

The project is built with [PlatformIO](https://platformio.org) — either the **PlatformIO IDE** extension for VS Code or the `pio` command line. All libraries, the ESP32 core and the partition scheme are defined in `platformio.ini` and installed automatically; nothing needs to be installed by hand. See [REQUIREMENTS.md](REQUIREMENTS.md) for the exact versions.

```sh
pio run                  # build
pio run -t upload        # build + flash over USB (settings are kept)
pio device monitor       # serial monitor (115200)
```

Each build also copies both release images to `release/` (`scripts/release_bins.py`), named with `FW_VERSION` from `include/config.h`.

In VS Code: open the repository folder and use **Build** / **Upload** / **Monitor** in the PlatformIO status bar or sidebar.

### Making a release

GitHub Actions (`.github/workflows/build.yml`) builds every push and pull request. To release:

1. Set `FW_VERSION` in `include/config.h` and the `CHANGELOG.md` section heading to the new version, commit, merge to `main`.
2. Tag it: `git tag v4.6 && git push origin v4.6` — the tag must equal `v` + `FW_VERSION`, or the build stops.
3. Actions creates a **draft** release with both `.bin` files and the version's CHANGELOG section. Check it on GitHub and press **Publish**.

---

## First-time Setup

1. Power on the device. It will create a Wi-Fi access point named **m5StickC-plus-xxxxxx**.
2. Connect to that network from your phone or computer (no password required).
3. Open a browser and go to **192.168.6.1**.
4. **Setup** — enter the TallyArbiter server IP and port (default: `4455`) and click Save. The device restarts and the access point comes back.
5. **Configure WiFi** — choose your network, enter its password and click Save. The device connects and the access point closes.

If the saved Wi-Fi network is not available (router still starting, another venue), the device keeps retrying it and opens the same access point after about 20 seconds. The access point closes by itself as soon as the network answers — no restart needed. A long press on the M5 button on the Settings screen opens it without erasing anything, and closes it again; **Exit** in the portal closes it too. Closed without a network, it stays closed until the network has been back once.

Saving **Setup** in the portal always restarts the device to apply the new TallyArbiter address.

---

## Controls

| Control | Action |
|---|---|
| **M5 button** — single click | Toggle between Tally screen and Settings screen |
| **M5 button** — double click | Toggle external LEDs on / off |
| **M5 button** — long press (~1 s) | On the Settings screen: open the setup access point (Wi-Fi settings are kept; closes after 2 min). Any screen: close it if open |
| **Action button** — single click | Show brightness bar (first click), then cycle brightness |
| **Action button** — long press | Run LED sequential check (Program → Preview → Aux → All) |
| **Grove pin G33** — short press | Open the setup access point (closes after 2 min; tally keeps running) |
| **Grove pin G33** — hold 3 s | Erase all settings and restart |

---

## Tally Screen

The main screen shows the current tally state as a full-screen background colour matching the TallyArbiter bus colour (red for Program, green for Preview, custom for Aux). The assigned device name is displayed in the centre.

- The last 6 characters of the device identifier (MAC suffix) are shown in the bottom-right corner for easy identification when multiple devices are in use.
- When external LEDs are disabled, a **LED OFF** indicator appears just above the device name.
- The screen rotates automatically when the device is turned upside down.

---

## Settings Screen

Shown automatically on disconnect and on startup until a connection is established. Displays:

- Connected Wi-Fi network name and IP address
- TallyArbiter server address and port
- Connection status (green = connected, red = not connected)
- Battery level: **Charging...** / **Charged.** / **nn%**
- Firmware version

Whenever the device is on the network, the configuration web portal is available at its IP address (shown on the Settings screen) — change the TallyArbiter server, see device info or upload new firmware without touching the device.

---

## External LED Wiring (optional)

| Signal | Pin |
|---|---|
| Program | G0 |
| Preview | G26 |
| Aux | G25 |

LED brightness tracks the display brightness setting. LEDs can be toggled on/off at runtime with a double click on the M5 button without affecting the tally display.

---

## OTA Firmware Updates

### Via web portal

1. Navigate to the device IP address (shown on the Settings screen) from any browser on the same network.
2. Go to **Update** in the portal menu and upload `GP_tally_M5StickC_v<ver>.bin` (not the `_full` file).

### Via PlatformIO

Use the `-ota` environment and pass the device IP address:

```sh
pio run -e m5stickc-ota -t upload --upload-port 192.168.1.50
```

- Hostname: `m5StickC-xxxxxx` / `m5StickC-plus-xxxxxx`
- Password: `tallyarbiter` (already set in `platformio.ini`)

---

## Project Structure

```
platformio.ini      — build environments, ESP32 core and library versions
boards/             — StickC board definition (same for StickC-Plus)
scripts/            — release_bins.py: versioned images to release/
.github/workflows/  — build on push / PR, draft release on a v* tag
include/
├── config.h        — pins, colours, timing, network constants, NVS keys
└── log.h           — LOGI / LOGE / LOGD
src/
├── main.cpp        — setup() / loop(): ties the modules together
├── board.*         — which StickC this is (detected at run time), listener name
├── settings.*      — everything kept in NVS
├── net.*           — Wi-Fi, WiFiManager setup AP / web config, OTA
├── tally.*         — TallyArbiter client (Socket.IO) -> tally state
├── leds.*          — external tally LEDs
├── ui.*            — display, buttons, animations
└── power.*         — battery, CPU clock
```

One firmware source for both boards: M5Unified detects the board at run time, so the code has no per-board `#ifdef`s.

Older versions built with the Arduino IDE are available as git tags `v4.5.1`, `v4.5.2`, `v4.5.3`.

---

## License

MIT
