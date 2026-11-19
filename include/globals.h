// ============================================================================
// globals.h — Shared declarations for all source files
//
// Global variables are *defined* in main.cpp and declared extern here.
// Every function shared between source files has its prototype here
// (this replaces the forward-declaration block the Arduino IDE required).
// ============================================================================
#pragma once

#pragma GCC optimize ("-O2")

#include <Arduino.h>
#include "board.h"

#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <PinButton.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <Preferences.h>

#include "config.h"

// External LED output enable
#define TALLY_EXTRA_OUTPUT true

// ============================================================================
// Firmware version
// ============================================================================
extern String version;

// ============================================================================
// User config (main.cpp)
// ============================================================================
extern bool      LAST_MSG;
extern IPAddress apIP;
extern char      tallyarbiter_host[40];
extern char      tallyarbiter_port[6];

// ============================================================================
// Board-specific device name strings
// ============================================================================
extern String listenerDeviceName;
extern String listenerDeviceHW;

// ============================================================================
// External LED pin assignments & PWM config
// ============================================================================
#if TALLY_EXTRA_OUTPUT
  #if defined(STICK_C_PLUS2)
    constexpr int led_program = 19;
  #else
    constexpr int led_program = 0;
  #endif
  constexpr int led_preview = 26;
  constexpr int led_aux     = 25;
  constexpr int freq        = 5000;
  constexpr int resolution  = 8;
  extern int dutyCycle;
#endif

// ============================================================================
// Tally state
// ============================================================================
extern String prevType;
extern String prevLedType;
extern String actualType;
extern String actualColor;
extern int    actualPriority;

// ============================================================================
// TallyArbiter / socket
// ============================================================================
extern WebSocketsClient socket;
extern JsonDocument     jBusOptions;
extern JsonDocument     jDevices;
extern JsonDocument     jDeviceStates;
extern String DeviceId;
extern String DeviceName;
extern String LastMessage;

extern bool taConnected;
extern bool everConnected;
extern bool busOptionsReady;
extern bool userRequestedSettings;

// ============================================================================
// General state
// ============================================================================
extern bool networkConnected;
extern int  currentScreen;
extern int  currentBrightness;
extern int  currentRotation;

extern WiFiManager wm;
extern bool        portalRunning;

extern float accX, accY, accZ;

// ============================================================================
// Animation state machines
// ============================================================================
#if TALLY_EXTRA_OUTPUT
  extern bool          ledCheckActive;
  extern int           ledCheckStep;
  extern unsigned long ledCheckLastMs;
  extern int           ledCheckOriginScreen;
  extern bool          ledsEnabled;
#endif

extern bool          flashActive;
extern int           flashStep;
extern unsigned long flashLastMs;

extern bool          reassignActive;
extern int           reassignStep;
extern unsigned long reassignLastMs;

extern bool          resetWaiting;
extern bool          resetHolding;
extern unsigned long resetPressedMs;

extern bool          brightnessOsdActive;
extern unsigned long brightnessOsdLastMs;
extern int           brightnessOsdOriginScreen;

// ============================================================================
// Battery cache
// ============================================================================
struct BatteryInfo {
  int           level      = 0;
  bool          isCharging = false;  // current flowing into battery
  bool          isCharged  = false;  // full and on charger (current == 0 but USB present)
  unsigned long lastRead   = 0;
};
extern BatteryInfo batteryCache;

// ============================================================================
// Reconnection / IMU / Preferences / Buttons
// ============================================================================
extern unsigned long lastReconnectAttempt;
extern int           reconnectDelay;
extern unsigned long lastImuRead;
extern Preferences   preferences;
extern PinButton     btnM5;
extern PinButton     btnAction;

// ============================================================================
// Function prototypes
// ============================================================================

// main.cpp
bool animationActive();

// screen.cpp
void paintSettingsScreen();
void paintTallyScreen();
void repaintCurrentScreen();
void switchToSettings(bool byUser = false);
void switchToTally();
void showSettings();
void showDeviceInfo();
void evaluateMode();

// buttons.cpp
void updateBrightness();
void showBrightnessOsd();
void brightnessTick();
void ledSequentialCheck();
void resetTick();
#if TALLY_EXTRA_OUTPUT
void ledCheckTick();
#endif

// tally.cpp
void connectToServer();
void ws_emit(String event, const char *payload);
void wsEvent(WStype_t type, uint8_t *payload, size_t length);
void onSocketConnect();
void onSocketDisconnect();
void onSocketEvent(String raw);
void socket_Flash();
void socket_Reassign(String oldDeviceId, String newDeviceId);
void processTallyData();
void getBusInfo(const String &busId, String &outType, String &outColor, int &outPriority);
void SetDeviceName();

// network.cpp
void connectToNetwork();
void saveParamCallback();
void saveConfigCallback();
String getParam(String name);
void WiFiEvent(WiFiEvent_t event);
void handleReconnection();
void checkPendingRestart();
void optimizeWiFiPower();

// leds.cpp
void externalLed();
void toggleLeds();
void set_dutyCycle();
void flashTick();
void reassignTick();

// power.cpp
void setPerformanceMode(bool high);
void updateBatteryInfo();

// m5hal.cpp
void logger(String strLog, String strType);
void m5_begin();
void m5_configInitialScreen();
void m5_configSettingsScreen();
void m5_setRotation(uint_fast8_t rotation);
void m5_println();
void m5_println(const String &s);
void m5_println(const char c[]);
void m5_print(const char str[]);
void m5_setTextColor(uint16_t color);
void m5_setTextColor(uint16_t fgcolor, uint16_t bgcolor);
void m5_fillScreen(uint32_t color);
void m5_fillRect(int x, int y, int w, int h, uint32_t color);
void m5_drawRect(int x, int y, int w, int h, uint32_t color);
uint16_t m5_color565(uint8_t r, uint8_t g, uint8_t b);
uint8_t  m5_BtnA_pressedFor(uint32_t ms);
int  m5_displayWidth();
int  m5_displayHeight();
int  lcdBrightness();
int  lcdBrightnessPlus2();
void configureDisplayToEvaluateMode();
void configureDisplayToCheckReset();
void ledCheckScreen(uint32_t bgColor, uint16_t textColor, String label, String pinNum, int stepNum, int totalSteps);
