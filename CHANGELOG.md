# Changelog

## [4.5.1] — 2026-03-16

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
