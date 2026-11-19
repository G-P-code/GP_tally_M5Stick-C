# Changelog

All notable changes to GP Tally M5StickC are documented here.

---

## [Unreleased]

### Changed
- **Build system migrated from Arduino IDE to PlatformIO** — `platformio.ini` with environments `m5stickc`, `m5stickc-plus`, `m5stickc-plus2` and matching `-ota` variants. ESP32 core, partition scheme and all library versions are pinned, so builds are reproducible without manual library installation.
- **ESP32 platform** — pioarduino `55.03.313` (Arduino-ESP32 core 3.3.13).
- **Board definitions** — `boards/` contains definitions for StickC, StickC-Plus and StickC-Plus2 (not shipped by pioarduino).
- **Source layout** — sketch tabs (`*.ino`) converted to regular C++ (`src/*.cpp`). Shared globals and prototypes moved to `include/globals.h`, board detection to `include/board.h`. No functional changes.
- **Docs** — README and REQUIREMENTS updated for PlatformIO. Library versions in docs now match the versions actually used (WebSockets 2.7.2, not 2.3.4).

### Added
- **Close the setup AP from the device** — M5 button long press (or **Exit** in the portal). Closed without a network, the AP stays closed until the network has been back once.

### Changed
- **Setup save always restarts** — saving the TallyArbiter settings restarts the device even if the values did not change.

### Fixed
- **Boot no longer hangs in the setup portal** — with the saved Wi-Fi unavailable, `autoConnect()` without a portal timeout waited in the AP for ever and never retried the network. Boot is now non-blocking: the station keeps retrying, the setup AP opens after 20 s and closes by itself when the network answers.
- **Wi-Fi events never fired** — `WiFiEvent()` compared core 3.x `ARDUINO_EVENT_*` ids with ESP-IDF `IP_EVENT_*` / `WIFI_EVENT_*` values (different numbers), so a lost network was never noticed. Station state now comes from the interface's event bits; drops are logged with their reason.
- **Wi-Fi reconnect** — a drop the core's auto-reconnect does not recover within 5 s is retried every 10 s. Joins the strongest AP when several share the SSID.
- **Stale tally on a dead link** — the Socket.IO ping was answered but its absence never checked, so a silently dropped link (Wi-Fi gone, server unplugged) kept the last tally colour on screen. No packet for pingInterval + pingTimeout now drops the connection and clears the tally.
- **TallyArbiter reconnect** — `socket.begin()` was called again on every retry on top of the library's own 500 ms reconnect, so the backoff never applied. The library now reconnects alone, with its interval doubling up to 30 s while the server is away (each attempt can block the loop up to 5 s).
- **G33 setup portal no longer blocks** — the tally kept running only after the 120 s portal closed; it is now non-blocking.
- **Web portal and setup AP on port 80** — never started together any more.
- **Portal form** — a reload or GET of `/paramsave` no longer saves empty values; a blank field keeps the stored value; the port is validated; the form shows the stored values after a save; no restart when nothing changed.
- **Firmware update page** — the portal menu now has **Update** (the README already described it).
- **TX power** — switching between 8.5 and 19.5 dBm has 10 dB hysteresis instead of flipping at −50 dBm.
- **Brightness** — the cycle stops at 100 instead of 101.
- **"Erase WiFi config" showed "An error occurred" and erased nothing** — on core 3.x the driver refuses to erase before the STA interface has started. A replacement `/erase` handler (and the M5 5 s / G33 3 s resets) now start the STA, wait for it, then erase.
- **Web portal could not be reopened after "Exit"** — WiFiManager stopped it but the `portalRunning` flag stayed set; the flag is now read from the library.
- **CHANGELOG** — duplicate 4.5.3 section merged.

---

## [4.5.3] — 2026-03-18

### Added
- **Device identifier on tally screen** — last 6 chars of the listener device name (MAC suffix, e.g. `53f278`) shown in bottom-right corner in small text. Matches `DeviceName` colour during active tally, near-invisible dark grey when idle. Hidden during brightness OSD.
- **Brightness OSD preview** — first Action button click shows the brightness bar without changing the level. Subsequent clicks while the OSD is visible cycle brightness as before.
- `DEFAULT_BRIGHTNESS 51` constant — sets ~50% brightness on first boot (no saved preference). The adjustable range `START_BRIGHTNESS`–`MAX_BRIGHTNESS` is unchanged.

### Changed
- **Socket.IO library replaced** — `timum-viw/SocketIoClient` (abandoned since 2021) replaced with a direct Socket.IO v2 implementation over `links2004/WebSockets`. Endpoint: `/socket.io/?EIO=4&transport=websocket`.
- **JSON library replaced** — `Arduino_JSON` replaced with `ArduinoJson` v7 (Benoît Blanchon). Faster parsing, lower RAM usage, type-safe API, actively maintained.
- **Cleaner event dispatch** — `socket_event()` replaced with `wsEvent()` (WebSocket/Engine.IO) and `onSocketEvent()` (Socket.IO application layer).
- **Bus lookup consolidated** — `getBusTypeById()` / `getBusColorById()` / `getBusPriorityById()` replaced with single `getBusInfo()`.
- **`btnM5` single click** — changed from `isClick()` to `isSingleClick()` so double-click is always handled independently without triggering a screen toggle.
- **WiFi config preserved on portal save** — `saveParamCallback()` no longer calls `ESP.restart()` directly. Restart is deferred to `saveConfigCallback()` (after WiFi credentials are saved) or a 2 s timeout fallback via `checkPendingRestart()`.

### Removed
- `timum-viw/SocketIoClient` dependency
- `Arduino_JSON` dependency
- `strip_quot()`, `socket_Connected()`, `socket_Messaging()` (folded into new handlers)

---

## [4.5.2] — 2026-03-17

### Fixed
- **Reassign tally state** — tally state (`actualType`, `actualColor`, LEDs) is now cleared immediately in `socket_Reassign()` before the new device name is set. Previously the stale colour from the previous device persisted until the next `device_states` event arrived.
- **Race condition on reassign** — `device_states` from TA can arrive before `devices` event completes device name update. Tally state is now reset at the correct point in the reassign sequence so stale state never bleeds into the new device display.
- **`bus_options` race on connect** — TA sends `device_states` before `bus_options` on initial connect. Added `busOptionsReady` flag so `processTallyData()` is deferred until bus colour and priority data is available. Buffered `DeviceStates` are processed immediately when `bus_options` arrives.
- **Device name update** — screen repaints immediately when TallyArbiter assigns a new device name.

---

## [4.5.1] — 2026-03-17

### Added
- **Multi-file sketch** — codebase split into logical modules for maintainability: `screen.ino`, `buttons.ino`, `tally.ino`, `network.ino`, `leds.ino`, `power.ino`, `m5hal.ino`, `config.h`
- **LED toggle** — M5 button double-click enables/disables external LEDs at runtime. A small **LED OFF** indicator appears on the tally screen when LEDs are disabled
- **Settings screen on startup** — device now opens the Settings screen on boot and switches to tally automatically on first successful connection, eliminating the tally→settings flash on startup
- **Battery status auto-update** — Settings screen refreshes battery level without manual navigation
- **Three-state battery indicator** — displays `Charging...`, `Charged.`, or `nn%`
- **Exponential backoff reconnection** — reconnect delay doubles from 1 s up to 30 s
- **Adaptive Wi-Fi TX power** — reduces transmit power when signal is strong to save battery
- **Adaptive CPU frequency** — switches between 80 MHz (idle) and 160 MHz (active)
- **LED state independence** — external LEDs update regardless of current screen or animation state

### Fixed
- **WiFiManager portal save** — saving new TallyArbiter host/port via web portal now triggers `ESP.restart()` so the new address takes effect immediately
- **Slow boot after config save** — added `wm.setConnectTimeout(15)` so WiFi connect attempt takes at most 15 s before opening the AP portal
- **Battery voltage formula** — corrected for both AXP192 (StickC/Plus) and Plus2; upper limit changed to 4.2 V
- **USB detection on StickC-Plus** — switched from `GetVinVoltage()` to `GetVBusVoltage()` which correctly reflects USB connection on this variant
- **Charging detection on AXP192** — replaced unreliable `GetBatCurrent()` with `GetVBusVoltage()` + SoC threshold logic
- **Plus2 display brightness** — implemented `StickCP2.Display.setBrightness()` replacing the previously unimplemented TODO
- **Plus2 IMU API** — corrected `M5.IMU.Init()` / `getAccelData()` to `M5.Imu.begin()` / `getAccel()` for M5Unified compatibility
- **Plus2 font API** — replaced deprecated `setFreeFont()` with `setFont()` for M5GFX
- **Dead code removed** — `configureDisplayToShowDeviceInfo()` (duplicate of `configureDisplayToEvaluateMode()`) and `#include <ESPmDNS.h>` removed

### Changed
- All `#define` constants and NVS preference keys consolidated in `config.h`
- `MIN_DUTY` / `MAX_DUTY` duplicate definition resolved
- `logger()` calls for portal save callback promoted from `info-quiet` to `info` for easier diagnostics

