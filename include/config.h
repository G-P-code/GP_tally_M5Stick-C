// ============================================================================
// config.h — Constants: firmware, pins, colours, timing, network
// ============================================================================
#pragma once

#include <stdint.h>

#define FW_VERSION "4.6"

// ----------------------------------------------------------------------------
// Pins (same on StickC and StickC-Plus)
// ----------------------------------------------------------------------------
constexpr int PIN_TRIGGER        = 33;  // grove: short press = setup AP, hold = erase WiFi
constexpr int PIN_LED_PREVIEW    = 26;
constexpr int PIN_LED_AUX        = 25;
constexpr int PIN_LED_PROGRAM    = 0;

// ----------------------------------------------------------------------------
// External LED PWM
// ----------------------------------------------------------------------------
constexpr int LED_PWM_FREQ = 5000;
constexpr int LED_PWM_BITS = 8;
constexpr int LED_MIN_DUTY = 8;
constexpr int LED_MAX_DUTY = 250;

// ----------------------------------------------------------------------------
// Brightness (percent; one value drives the display and the external LEDs)
// ----------------------------------------------------------------------------
constexpr int BRIGHTNESS_MIN     = 11;
constexpr int BRIGHTNESS_MAX     = 100;
constexpr int BRIGHTNESS_DEFAULT = 51;
constexpr int BRIGHTNESS_STEP    = 10;
constexpr int LCD_LEVEL_MIN      = 20;   // display level (0-255) at BRIGHTNESS_MIN

// ----------------------------------------------------------------------------
// Tally screen colours (RGB565)
// ----------------------------------------------------------------------------
// On an active tally the text is a shade of the tally colour, this % of the
// way to white (dark colour) or black (light colour).
constexpr int      TALLY_NAME_CONTRAST = 55;
constexpr int      TALLY_ID_CONTRAST   = 25;
constexpr uint16_t COLOR_IDLE_NAME     = 0x7BEF;   // DARKGREY
constexpr uint16_t COLOR_IDLE_ID       = 0x52AA;   // a little darker than the name

// ----------------------------------------------------------------------------
// Timing (ms)
// ----------------------------------------------------------------------------
constexpr uint32_t BUTTON_HOLD_MS      = 600;    // long press
constexpr uint32_t TRIGGER_DEBOUNCE_MS = 50;
constexpr uint32_t TRIGGER_HOLD_MS     = 3000;   // G33 held: erase WiFi
constexpr uint32_t OSD_MS              = 1500;   // brightness bar
constexpr uint32_t FLASH_STEP_MS       = 500;    // "flash" from TallyArbiter: white / black
constexpr int      FLASH_STEPS         = 6;
constexpr uint32_t REASSIGN_STEP_MS    = 200;    // reassigned: red / black
constexpr int      REASSIGN_STEPS      = 4;
constexpr uint32_t LED_CHECK_STEP_MS   = 1000;
constexpr int      LED_CHECK_STEPS     = 4;
constexpr uint32_t IMU_READ_MS         = 100;
constexpr uint32_t BATTERY_READ_MS     = 5000;

// ----------------------------------------------------------------------------
// TallyArbiter
// ----------------------------------------------------------------------------
#define TA_DEFAULT_HOST "0.0.0.0"
#define TA_DEFAULT_PORT "4455"
constexpr uint32_t TA_RETRY_MIN_MS     = 1000;   // reconnect interval, doubles while TA is away
constexpr uint32_t TA_RETRY_MAX_MS     = 30000;
constexpr uint32_t TA_PING_INTERVAL_MS = 25000;  // Socket.IO v4 defaults, replaced by the
constexpr uint32_t TA_PING_TIMEOUT_MS  = 20000;  //   server's values from the open packet
constexpr uint32_t TA_PING_MARGIN_MS   = 5000;   // slack before a silent link counts as dead

// ----------------------------------------------------------------------------
// WiFi / setup portal / OTA
// ----------------------------------------------------------------------------
#define SETUP_AP_IP  192, 168, 6, 1
#define OTA_PASSWORD "tallyarbiter"
constexpr uint32_t WIFI_AP_AFTER_MS          = 20000;  // no network this long: open the setup AP
constexpr uint32_t WIFI_STA_RESTART_MS       = 10000;  // restart a station the setup AP switched off
constexpr uint32_t SETUP_AP_MANUAL_TIMEOUT_S = 120;    // setup AP opened by hand (M5 / G33) closes after this
constexpr int      WIFI_POWER_SAVE_RSSI      = -50;    // dBm: low TX power above this
constexpr int      WIFI_POWER_HYSTERESIS     = 10;     // dB: back to full power below RSSI - this
constexpr uint32_t RESTART_AFTER_SAVE_MS     = 2000;   // portal "Setup" save -> restart

// ----------------------------------------------------------------------------
// NVS keys (namespace kept from earlier versions so saved settings survive)
// ----------------------------------------------------------------------------
#define PREF_NAMESPACE   "tally-arbiter"
#define PREF_DEVICE_ID   "deviceid"
#define PREF_DEVICE_NAME "devicename"
#define PREF_TA_HOST     "taHost"
#define PREF_TA_PORT     "taPort"
#define PREF_BRIGHTNESS  "brightness"
