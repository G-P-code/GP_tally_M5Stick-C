// ************************************************************************************************************** //
// Based on https://github.com/josephdadams/TallyArbiter/listener_clients/m5stickc-listener/m5stickc-listener.ino //
// by Joseph Adams                                                                                                //
// ************************************************************************************************************** //

#include "globals.h"

String version = "4.5.3";

// ============================================================================
// USER CONFIG — change before flashing
// ============================================================================
bool LAST_MSG = false;              // true = show last log message on tally screen
IPAddress apIP(192, 168, 6, 1);    // captive portal IP
char tallyarbiter_host[40] = "0.0.0.0";
char tallyarbiter_port[6]  = "4455";
// ============================================================================

// ============================================================================
// Board-specific device name strings
// ============================================================================
#if defined(STICK_C_PLUS2)
  String listenerDeviceName = "m5StickC-plus2-";
  String listenerDeviceHW   = "M5StickC-Plus2";
#elif defined(STICK_C_PLUS)
  String listenerDeviceName = "m5StickC-plus-";
  String listenerDeviceHW   = "M5StickC-Plus";
#else
  String listenerDeviceName = "m5StickC-";
  String listenerDeviceHW   = "M5StickC";
#endif

// ============================================================================
// External LED pin assignments & PWM config
// ============================================================================
#if TALLY_EXTRA_OUTPUT
  // Pin numbers and PWM config are constexpr in globals.h
  int dutyCycle          = 0;
#endif

// ============================================================================
// Tally state
// ============================================================================
String prevType    = "";               // last displayed tally type (display only)
String prevLedType = "";               // last applied LED tally type (LED only)
String actualType  = "";
String actualColor = "";
int    actualPriority = 0;

// ============================================================================
// TallyArbiter / socket variables
// ============================================================================
WebSocketsClient socket;

// ArduinoJson documents — bus options and device data
// JsonDocument is reused; device_states is re-parsed each time
JsonDocument jBusOptions;   // bus definitions (type, color, priority)
JsonDocument jDevices;      // device list (id, name)
JsonDocument jDeviceStates; // current tally state per device per bus
String DeviceId   = "unassigned";
String DeviceName = "Unassigned";
String LastMessage = "";

bool taConnected          = false;
bool everConnected        = false;  // set true on first successful CONNECT
bool busOptionsReady      = false;  // set true once bus_options received from TA
bool userRequestedSettings = false;   // true = user opened settings manually

// ============================================================================
// General state
// ============================================================================
bool networkConnected  = false;
int  currentScreen     = 0;           // 0 = tally, 1 = settings
int  currentBrightness = DEFAULT_BRIGHTNESS;  // overridden by saved preference on boot
int  currentRotation   = 1;

WiFiManager wm;
bool portalRunning = false;

float accX = 0, accY = 0, accZ = 0;

// ============================================================================
// Animation state machines
// ============================================================================
#if TALLY_EXTRA_OUTPUT
  bool          ledCheckActive       = false;
  int           ledCheckStep         = 0;
  unsigned long ledCheckLastMs       = 0;
  int           ledCheckOriginScreen = 0;
  bool          ledsEnabled          = true;   // toggled by btnM5 double-click
#endif

bool          flashActive    = false;
int           flashStep      = 0;
unsigned long flashLastMs    = 0;

bool          reassignActive  = false;
int           reassignStep    = 0;
unsigned long reassignLastMs  = 0;

bool          resetWaiting    = false;
bool          resetHolding    = false;
unsigned long resetPressedMs  = 0;

bool          brightnessOsdActive       = false;
unsigned long brightnessOsdLastMs       = 0;
int           brightnessOsdOriginScreen = 0;

// ============================================================================
// Battery cache
// ============================================================================
BatteryInfo batteryCache;   // struct defined in globals.h

// ============================================================================
// Reconnection
// ============================================================================
unsigned long lastReconnectAttempt = 0;
int           reconnectDelay       = RECONNECT_BASE_DELAY;

// ============================================================================
// IMU throttle
// ============================================================================
unsigned long lastImuRead = 0;

// ============================================================================
// Preferences
// ============================================================================
Preferences preferences;

// ============================================================================
// Buttons
// ============================================================================
const MultiButtonConfig BTN_CONFIG = { 20, 250, 600 };
PinButton btnM5(37, &BTN_CONFIG);
PinButton btnAction(39, &BTN_CONFIG);

// ============================================================================
// Animation guard — true while any animation is running
// ============================================================================
bool animationActive() {
  bool active = flashActive || reassignActive || brightnessOsdActive;
#if TALLY_EXTRA_OUTPUT
  active = active || ledCheckActive;
#endif
  return active;
}

// ============================================================================
// setup()
// ============================================================================
void setup() {
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  unsigned long serialWaitStart = millis();
  while (!Serial && (millis() - serialWaitStart < 2000));

  logger("Initializing " + listenerDeviceHW + ".", "info-quiet");

  setCpuFrequencyMhz(80);
  btStop();

  byte mac[6];
  Network.macAddress(mac);
  // Pad each byte to 2 hex digits so the suffix is always exactly 6 chars
  auto hexByte = [](uint8_t b) -> String {
    return (b < 0x10 ? "0" : "") + String(b, HEX);
  };
  listenerDeviceName += hexByte(mac[3]) + hexByte(mac[4]) + hexByte(mac[5]);
  wm.setHostname(listenerDeviceName.c_str());

  m5_begin();

  preferences.begin(PREF_NAMESPACE, false);
  logger("Reading preferences", "info-quiet");
  if (preferences.getString(PREF_DEVICE_ID, "").length() > 0)
    DeviceId = preferences.getString(PREF_DEVICE_ID, "");
  if (preferences.getString(PREF_DEVICE_NAME, "").length() > 0)
    DeviceName = preferences.getString(PREF_DEVICE_NAME, "");
  if (preferences.getString(PREF_TA_HOST, "").length() > 0) {
    String h = preferences.getString(PREF_TA_HOST, "");
    logger("Setting TallyArbiter host as " + h, "info-quiet");
    h.toCharArray(tallyarbiter_host, 40);
  }
  if (preferences.getString(PREF_TA_PORT, "").length() > 0) {
    String p = preferences.getString(PREF_TA_PORT, "");
    logger("Setting TallyArbiter port as " + p, "info-quiet");
    p.toCharArray(tallyarbiter_port, 6);
  }
  if (preferences.getInt(PREF_BRIGHTNESS, 0) > 0)
    currentBrightness = preferences.getInt(PREF_BRIGHTNESS, DEFAULT_BRIGHTNESS);
  preferences.end();

#if defined(STICK_C_PLUS2)
  M5.Imu.begin();
#else
  M5.IMU.Init();
#endif
  m5_setRotation(1);
  m5_fillScreen(TFT_BLACK);
  m5_configInitialScreen();
  m5_setTextColor(WHITE, BLACK);
  m5_println("booting device " + hexByte(mac[3]) + hexByte(mac[4]) + hexByte(mac[5]) + " ...");

  logger("Tally Arbiter " + listenerDeviceHW + " Listener Client booting.", "info");
  logger("Listener device name: " + listenerDeviceName, "info");

  // Non-blocking: the station keeps retrying in networkTick(), the setup AP
  // opens by itself when there is no network.
  connectToNetwork();

  ArduinoOTA.setHostname(listenerDeviceName.c_str());
  ArduinoOTA.setPassword("tallyarbiter");
  ArduinoOTA
    .onStart([]() {
      String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
      Serial.println("Start updating " + type);
    })
    .onEnd([]()   { Serial.println("\nEnd"); })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", progress / (total / 100));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if      (error == OTA_AUTH_ERROR)    logger("Auth Failed",    "error");
      else if (error == OTA_BEGIN_ERROR)   logger("Begin Failed",   "error");
      else if (error == OTA_CONNECT_ERROR) logger("Connect Failed", "error");
      else if (error == OTA_RECEIVE_ERROR) logger("Receive Failed", "error");
      else if (error == OTA_END_ERROR)     logger("End Failed",     "error");
    });
  // ArduinoOTA.begin() runs in networkTick() once the network is up.

#if TALLY_EXTRA_OUTPUT
  set_dutyCycle();
  ledcAttach(led_program, freq, resolution);
  ledcAttach(led_preview, freq, resolution);
  ledcAttach(led_aux,     freq, resolution);
#endif

  connectToServer();
  switchToSettings();  // stay on settings until first successful connection
}

// ============================================================================
// loop()
// ============================================================================
void loop() {
  networkTick();

  ArduinoOTA.handle();
  socket.loop();
  btnM5.update();
  btnAction.update();
  M5.update();

  // IMU — read at reduced frequency, act on rotation change only
  unsigned long now = millis();
  if (now - lastImuRead >= IMU_READ_INTERVAL) {
#if defined(STICK_C_PLUS2)
    M5.Imu.getAccel(&accX, &accY, &accZ);
#else
    M5.IMU.getAccelData(&accX, &accY, &accZ);
#endif
    lastImuRead = now;

    int newRotation = currentRotation;
    if      (accX < -0.70) newRotation = 3;
    else if (accX >  0.70) newRotation = 1;

    if (newRotation != currentRotation && !animationActive()) {
      currentRotation = newRotation;
      m5_setRotation(currentRotation);
      prevType = "__invalidated__";
      repaintCurrentScreen();
      btnM5.update();
      btnAction.update();
      return;
    }
  }

  // WiFi reset: hold btnM5 for 5 s
  if (m5_BtnA_pressedFor(5000)) {
    logger("resetSettings()", "info");
    eraseWifiConfig();
    ESP.restart();
  }

  // Screen toggle: single click btnM5
  if (btnM5.isSingleClick()) {
    if (currentScreen == 0) {
      switchToSettings(true);
    } else {
      userRequestedSettings = false;
      switchToTally();
    }
  }

  // LED toggle: double click btnM5
#if TALLY_EXTRA_OUTPUT
  if (btnM5.isDoubleClick()) toggleLeds();
#endif

  // Close the setup AP: long press btnM5 (hold 5 s still erases WiFi)
  if (btnM5.isLongClick() && setupApActive()) closeSetupAp();

  // Brightness cycle: single click btnAction
  if (btnAction.isSingleClick()) updateBrightness();

  // LED sequential check: long press btnAction
  if (btnAction.isLongClick()) ledSequentialCheck();

  // Tick functions
#if TALLY_EXTRA_OUTPUT
  ledCheckTick();
#endif
  flashTick();
  reassignTick();
  resetTick();
  brightnessTick();

  // Maintenance
  checkPendingRestart();
  taTick();
  optimizeWiFiPower();
  updateBatteryInfo();
  setPerformanceMode(animationActive() || portalRunning || setupApActive() || !taConnected);
}
