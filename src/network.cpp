// ============================================================================
// network.cpp — WiFi connection, WifiManager portal, reconnection, TX power
//
// WiFiManager pitfalls handled here (same fixes as x32_talkback / QLab remote):
//
//  * Boot never blocks. autoConnect() with no portal timeout sat in the setup AP
//    for ever when the saved network was simply not there yet (router still
//    booting, other venue) and never tried that network again. The station now
//    keeps retrying; the setup AP opens after WIFI_AP_AFTER_MS without a
//    network and closes by itself as soon as the network answers.
//  * Station state comes from the core's event bits (connected AND has IP),
//    not WiFi.status(): after some disconnect reasons status() stays at
//    WL_CONNECTED for minutes and nothing ever reconnects.
//  * Reconnecting is the core's auto-reconnect. Only when opening the setup AP
//    switched the station off is it started again, so a returning network is
//    still noticed and the AP closes by itself.
//  * A phone on the setup AP gets the radio: WiFiManager's network scan fails
//    while the station is trying to connect (the portal then lists no networks).
//  * Web portal (startWebPortal) and setup AP portal (startConfigPortal) both
//    bind port 80 — they are never run together.
//  * The AP state is read from the library, never remembered: WiFiManager
//    closes the portal by itself after a successful WiFi save.
//  * /paramsave answers any request, so a page reload or a GET repeats the
//    "save" with empty fields — those are ignored. A blank field means "keep
//    what is stored", and the form is refilled after every save (WiFiManager
//    empties the parameter values from each request).
// ============================================================================

#include "globals.h"

static WiFiManagerParameter *custom_taServer;
static WiFiManagerParameter *custom_taPort;

// Station / AP state (owned by networkTick)
static bool          staUp        = false;
static bool          apUp         = false;
static bool          apManual     = false;   // opened with G33 (has its own timeout)
static bool          noCreds      = false;   // nothing saved: AP at once, nothing to retry
static bool          retryPaused  = false;   // phone on the setup AP
static bool          otaStarted   = false;
static bool          apSuppressed = false;   // AP closed by hand without network: no auto-reopen
static unsigned long staDownSince = 0;
static unsigned long lastWifiKick = 0;

// Last disconnect reason, written from the WiFi event task, logged from loop()
static volatile uint8_t  discReason = 0;
static volatile uint32_t discCount  = 0;

// Refresh the settings screen with the new network state — but not on top of
// an animation (LED check, brightness OSD), which restores the screen itself.
static void repaintSettings() {
  if (currentScreen == 1 && !animationActive()) paintSettingsScreen();
}

// ----------------------------------------------------------------------------
// Portal form helpers
// ----------------------------------------------------------------------------

// Put the live values back into the form fields.
static void refreshParamValues() {
  custom_taServer->setValue(tallyarbiter_host, sizeof(tallyarbiter_host) - 1);
  custom_taPort->setValue(tallyarbiter_port, sizeof(tallyarbiter_port) - 1);
}

// ----------------------------------------------------------------------------
// Setup / web portal control
// ----------------------------------------------------------------------------

static void openSetupAp(bool manual) {
  if (wm.getConfigPortalActive()) { apUp = true; return; }
  stopWebPortal();                                  // both use port 80
  wm.setConfigPortalTimeout(manual ? SETUP_AP_MANUAL_TIMEOUT_S : 0);
  wm.startConfigPortal(listenerDeviceName.c_str()); // non-blocking
  apUp     = wm.getConfigPortalActive();
  apManual = manual && apUp;
  logger("Setup AP " + listenerDeviceName + " at http://" + apIP.toString() + "/" +
         (manual ? " (manual)" : " (no network)"), "info");
  repaintSettings();
}

void startWebPortal() {
  if (portalRunning || wm.getConfigPortalActive()) return;  // never together with the AP
  wm.startWebPortal();
  portalRunning = true;
}

void stopWebPortal() {
  if (!portalRunning) return;
  wm.stopWebPortal();
  portalRunning = false;
}

void openSetupApManual() { apSuppressed = false; openSetupAp(true); }

// M5 long press / portal "Exit". Without a network the AP is not opened again
// by itself until the network has come back once (G33 still opens it).
void closeSetupAp() {
  if (!wm.getConfigPortalActive()) return;
  logger("Setup AP closed by user", "info");
  wm.stopConfigPortal();
  // apUp edge is handled in networkTick()
}

// WiFi credentials erase that works on core 3.x: the driver refuses to erase
// until the STA interface has actually started, and WiFiManager's own /erase
// asks before that happens ("An error occurred", nothing erased).
bool eraseWifiConfig() {
  WiFi.enableSTA(true);
  const unsigned long t = millis();
  while (!WiFi.STA.started() && millis() - t < 2000) delay(10);
  const bool ok = WiFi.eraseAP();
  logger(String("WiFi config erase ") + (ok ? "OK" : "FAILED"), ok ? "info" : "error");
  return ok;
}

// Registered before WiFiManager's own routes, so this /erase wins.
static void handleErase() {
  const bool ok = eraseWifiConfig();
  wm.server->send(200, "text/html",
    String("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width'>"
           "<title>Erase WiFi</title></head><body style='font-family:sans-serif'><h3>") +
    (ok ? "WiFi config erased. Restarting..." : "Erasing WiFi config failed.") +
    "</h3></body></html>");
  if (ok) {
    delay(1000);
    ESP.restart();
  }
}

bool setupApActive()  { return apUp; }
bool stationUp()      { return staUp; }

// ----------------------------------------------------------------------------
// Connection
// ----------------------------------------------------------------------------

void connectToNetwork() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(listenerDeviceName.c_str());
  WiFi.setAutoReconnect(true);
  // Several APs with one SSID (venue / mesh): join the strongest, not the first found.
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
  WiFi.onEvent(WiFiEvent);

  custom_taServer = new WiFiManagerParameter("taHostIP",   "Tally Arbiter Server", tallyarbiter_host, 39);
  custom_taPort   = new WiFiManagerParameter("taHostPort", "Port",                 tallyarbiter_port, 5);
  wm.addParameter(custom_taServer);
  wm.addParameter(custom_taPort);

  wm.setSaveParamsCallback(saveParamCallback);
  wm.setSaveConfigCallback(saveConfigCallback);

  // "update" = firmware upload page (OTA from the browser)
  std::vector<const char *> menu = { "wifi", "param", "info", "update", "sep", "restart", "exit" };
  wm.setMenu(menu);
  wm.setClass("invert");
  wm.setTitle("GP Tally v" + version);
  wm.setHostname(listenerDeviceName.c_str());
  wm.setAPStaticIPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  wm.setConnectTimeout(15);              // after a WiFi save in the portal
  wm.setConfigPortalBlocking(false);     // boot and loop never block
  wm.setWebServerCallback([]() { wm.server->on("/erase", handleErase); });

  staDownSince = millis();
  if (wm.getWiFiIsSaved()) {
    m5_println("connecting...");
    logger("Connecting to saved WiFi", "info");
    WiFi.begin();                        // async; networkTick() does the rest
  } else {
    noCreds = true;
    m5_println("Configure on");
    m5_println("SSID: " + listenerDeviceName);
    logger("No saved WiFi, opening setup AP", "info");
    openSetupAp(false);
  }
}

// Called every loop() pass.
void networkTick() {
  const unsigned long now = millis();

  // Each new disconnect reason is logged once, not on every retry.
  {
    static uint32_t seen = 0;
    static uint8_t  lastLogged = 0;
    if (discCount != seen) {
      seen = discCount;
      const uint8_t r = discReason;
      if (r != lastLogged) {
        lastLogged = r;
        logger("WiFi dropped / failed: reason " + String(r) + " (" +
               WiFi.disconnectReasonName((wifi_err_reason_t)r) + ")", "info");
      }
    }
    if (staUp) lastLogged = 0;
  }

  // Web portal state — WiFiManager's own "Exit" stops it behind our back
  portalRunning = wm.getWebPortalActive();

  // Setup AP state — read from the library
  const bool apNow = wm.getConfigPortalActive();
  if (apNow != apUp) {
    apUp = apNow;
    logger(String("Setup AP ") + (apNow ? "up" : "closed"), "info");
    if (!apNow) {
      // Closed while there is no network (M5 long press, "Exit", timeout):
      // do not pop it straight back up.
      if (!staUp) apSuppressed = true;
      apManual = false;
      if (currentScreen == 1) startWebPortal();
    }
    repaintSettings();
  }

  // Station state — from the interface's event bits (associated AND has an IP)
  const bool up = WiFi.STA.connected() && WiFi.STA.hasIP();
  if (up != staUp) {
    staUp = up;
    networkConnected = up;
    if (up) {
      noCreds = false;
      apSuppressed = false;
      logger("Network connected: " + WiFi.SSID() + " " + WiFi.localIP().toString(), "info");
      if (!otaStarted) { ArduinoOTA.begin(); otaStarted = true; }
      // TallyArbiter: retry at once instead of waiting out the backoff
      reconnectDelay = RECONNECT_BASE_DELAY;
      socket.setReconnectInterval(reconnectDelay);
      if (apUp && !apManual) {                   // the real network is back
        wm.stopConfigPortal();
        apUp = false;
        logger("Setup AP closed, network is up", "info");
        if (currentScreen == 1) startWebPortal();
      }
    } else {
      staDownSince = now;
      logger("Network connection lost", "info");
    }
    repaintSettings();
  }

  // A phone on the setup AP: stop station retries so the portal can scan.
  {
    const bool pause = !staUp && apUp && WiFi.softAPgetStationNum() > 0;
    if (pause != retryPaused) {
      retryPaused = pause;
      WiFi.setAutoReconnect(!pause);
      if (pause) WiFi.disconnect();
      else       lastWifiKick = 0;              // retry at once
      logger(pause ? "Phone on setup AP: WiFi retries paused"
                   : "Setup AP free: WiFi retries resumed", "info");
    }
  }

  if (!staUp) {
    // Reconnecting is left to the core's auto-reconnect. The one exception:
    // opening the setup AP without a network switches the station off
    // (WiFiManager), and then nothing would notice the network coming back.
    const bool staOn = WiFi.getMode() & WIFI_MODE_STA;
    if (!noCreds && !retryPaused && !staOn &&
        (lastWifiKick == 0 || now - lastWifiKick > WIFI_RETRY_MS)) {
      lastWifiKick = now;
      logger("Station was off, starting it again", "info");
      WiFi.begin();
    }
    // No network for a while: offer the setup AP (station keeps trying).
    if (!apUp && !apSuppressed && (noCreds || now - staDownSince > WIFI_AP_AFTER_MS)) openSetupAp(false);
  } else {
    lastWifiKick = 0;
  }

  if (apUp || portalRunning) wm.process();
}

String getParam(String name) {
  if (wm.server && wm.server->hasArg(name)) return wm.server->arg(name);
  return "";
}

// Flag — set when TA params have been saved via portal, triggers restart
// after WiFiManager has finished its own save sequence.
static bool          pendingRestart   = false;
static unsigned long pendingRestartMs = 0;

void saveParamCallback() {
  logger("[CALLBACK] saveParamCallback fired", "info");

  // Page reload / GET of /paramsave: not a real save.
  if (wm.server && (wm.server->method() != HTTP_POST || wm.server->args() == 0)) {
    logger("Ignoring /paramsave without form data (page reload)", "info");
    refreshParamValues();
    return;
  }

  String str_taHost = getParam("taHostIP");
  String str_taPort = getParam("taHostPort");
  str_taHost.trim();
  str_taPort.trim();

  // Blank field = keep what is stored (autofill / re-rendered empty form).
  if (str_taHost.length() == 0) str_taHost = tallyarbiter_host;
  if (str_taPort.length() == 0) str_taPort = tallyarbiter_port;
  const long port = str_taPort.toInt();
  if (port < 1 || port > 65535) {
    logger("Invalid TA port '" + str_taPort + "', keeping " + String(tallyarbiter_port), "error");
    str_taPort = tallyarbiter_port;
  }

  logger("New TA host: " + str_taHost + ":" + str_taPort, "info");

  // Save to NVS
  preferences.begin(PREF_NAMESPACE, false);
  preferences.putString(PREF_TA_HOST, str_taHost);
  preferences.putString(PREF_TA_PORT, str_taPort);
  preferences.end();

  // Update runtime variables
  str_taHost.toCharArray(tallyarbiter_host, sizeof(tallyarbiter_host));
  str_taPort.toCharArray(tallyarbiter_port, sizeof(tallyarbiter_port));
  refreshParamValues();

  logger("TA params saved. Restart pending after WiFiManager completes.", "info");

  // Do NOT restart here — WiFiManager may still be saving WiFi credentials.
  // Set flag and restart from saveConfigCallback instead, which fires last.
  pendingRestart   = true;
  pendingRestartMs = millis();
}

void saveConfigCallback() {
  // Called by WiFiManager after WiFi credentials are saved.
  // This always fires after saveParamCallback, so it is safe to restart here.
  logger("WiFi config saved.", "info");
  noCreds = false;
  if (pendingRestart) {
    logger("Rebooting...", "info");
    delay(500);  // allow web server to send response to browser
    ESP.restart();
  }
}

// WiFi event task — only record, log from loop().
// (Core 3.x passes ARDUINO_EVENT_* ids; the ESP-IDF IP_EVENT_* / WIFI_EVENT_*
//  values used before are different numbers and never matched.)
void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    discReason = info.wifi_sta_disconnected.reason;
    discCount  = discCount + 1;
  }
}

// Check if a portal save triggered a pending restart.
// Fires 2 s after saveParamCallback if saveConfigCallback did not fire
// (i.e. user changed only TA params, not WiFi).
void checkPendingRestart() {
  if (!pendingRestart) return;
  if (millis() - pendingRestartMs < 2000) return;
  logger("Pending restart timeout — rebooting.", "info");
  delay(500);
  ESP.restart();
}

// Reduce TX power when signal is strong, checked every 10 s.
// Hysteresis: back to full power only once the signal is clearly weaker,
// so the power does not flip on every check near the threshold.
void optimizeWiFiPower() {
  if (!networkConnected) return;
  static unsigned long lastCheck = 0;
  static bool lowPower = false;
  unsigned long now = millis();
  if (now - lastCheck < 10000) return;
  lastCheck = now;
  const int rssi = WiFi.RSSI();
  if (!lowPower && rssi > WIFI_POWER_SAVE_RSSI) {
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    lowPower = true;
  } else if (lowPower && rssi < WIFI_POWER_SAVE_RSSI - WIFI_POWER_HYSTERESIS) {
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    lowPower = false;
  }
}
