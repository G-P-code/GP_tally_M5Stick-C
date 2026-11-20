// ============================================================================
// net.cpp — WiFi, WiFiManager setup AP / web config, OTA
//
//  * Boot never blocks. The core's auto-reconnect keeps trying the saved
//    network; the setup AP opens after WIFI_AP_AFTER_MS without one and closes
//    by itself when the network answers. Opening the AP without a network
//    switches the station off (WiFiManager), so it is started again here.
//  * Station state from the interface's event bits (associated AND has IP).
//  * A phone on the setup AP pauses station retries (the portal scan fails
//    while the station is connecting).
//  * Web config (startWebPortal) and setup AP (startConfigPortal) both use
//    port 80 — never together. Portal states are read from the library.
//  * /paramsave: a reload or GET is ignored, a blank field keeps its value.
//    A real save restarts the device.
//  * /erase replaced: core 3.x refuses to erase before the STA has started.
// ============================================================================

#include "net.h"

#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include "config.h"
#include "log.h"
#include "settings.h"

namespace {

WiFiManager           wm;
WiFiManagerParameter *pHost = nullptr;
WiFiManagerParameter *pPort = nullptr;
String                host;
const IPAddress       apAddress(SETUP_AP_IP);

bool     staUp        = false;
bool     apUp_        = false;
bool     apManual     = false;   // opened with G33 (has its own timeout)
bool     apSuppressed = false;   // closed by hand without a network: no auto-reopen
bool     noCreds      = false;   // nothing saved: AP at once
bool     retryPaused  = false;   // phone on the setup AP
bool     otaStarted   = false;
uint32_t staDownSince = 0;
uint32_t staRestartAt = 0;
uint32_t restartAt    = 0;       // pending restart after a portal save (0 = none)
uint32_t changeCount  = 0;

volatile uint8_t  discReason = 0;   // written from the WiFi event task
volatile uint32_t discCount  = 0;

void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    discReason = info.wifi_sta_disconnected.reason;
    discCount  = discCount + 1;
  }
}

void refreshForm() {
  const Settings &s = settings::get();
  pHost->setValue(s.taHost, sizeof(s.taHost) - 1);
  pPort->setValue(s.taPort, sizeof(s.taPort) - 1);
}

String arg(const char *name) {
  return (wm.server && wm.server->hasArg(name)) ? wm.server->arg(name) : String();
}

void onParamsSaved() {
  if (wm.server && (wm.server->method() != HTTP_POST || wm.server->args() == 0)) {
    LOGI("Ignoring /paramsave without form data (page reload)");
    refreshForm();
    return;
  }
  const Settings &s = settings::get();
  String newHost = arg("taHostIP");
  String newPort = arg("taHostPort");
  newHost.trim();
  newPort.trim();
  if (newHost.isEmpty()) newHost = s.taHost;
  if (newPort.isEmpty()) newPort = s.taPort;
  const long port = newPort.toInt();
  if (port < 1 || port > 65535) {
    LOGE("Invalid TallyArbiter port '%s', keeping %s", newPort.c_str(), s.taPort);
    newPort = s.taPort;
  }
  settings::saveTaServer(newHost, newPort);
  refreshForm();
  LOGI("TallyArbiter server saved: %s:%s, restarting", newHost.c_str(), newPort.c_str());
  // Not right away: WiFiManager may still be saving WiFi credentials, and the
  // browser should get its "saved" page.
  restartAt = millis() + RESTART_AFTER_SAVE_MS;
}

void onWifiSaved() { noCreds = false; }

void handleErase() {
  const bool ok = net::eraseWifi();
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

void webPortal(bool on) {
  const bool running = wm.getWebPortalActive();
  if (on && !running && !wm.getConfigPortalActive()) wm.startWebPortal();
  if (!on && running) wm.stopWebPortal();
}

void openAp(bool manual) {
  if (wm.getConfigPortalActive()) return;
  webPortal(false);                                    // both use port 80
  wm.setConfigPortalTimeout(manual ? SETUP_AP_MANUAL_TIMEOUT_S : 0);
  wm.startConfigPortal(host.c_str());                  // non-blocking
  apManual = manual;
  LOGI("Setup AP %s at http://%s/ (%s)", host.c_str(), apAddress.toString().c_str(),
       manual ? "manual" : "no network");
}

void setupOta() {
  ArduinoOTA.setHostname(host.c_str());
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() { LOGI("OTA update started"); });
  ArduinoOTA.onEnd([]()   { LOGI("OTA update done"); });
  ArduinoOTA.onError([](ota_error_t e) { LOGE("OTA error %u", (unsigned)e); });
}

// Lower TX power on a strong signal, with hysteresis; checked every 10 s.
void tuneTxPower(uint32_t now) {
  static uint32_t checkedAt = 0;
  static bool     low = false;
  if (now - checkedAt < 10000) return;
  checkedAt = now;
  const int rssi = WiFi.RSSI();
  if (!low && rssi > WIFI_POWER_SAVE_RSSI) {
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    low = true;
  } else if (low && rssi < WIFI_POWER_SAVE_RSSI - WIFI_POWER_HYSTERESIS) {
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    low = false;
  }
}

void logDisconnects() {
  static uint32_t seen = 0;
  static uint8_t  logged = 0;
  if (discCount != seen) {
    seen = discCount;
    const uint8_t r = discReason;
    if (r != logged) {
      logged = r;
      LOGI("WiFi dropped / failed: reason %u (%s)", r, WiFi.disconnectReasonName((wifi_err_reason_t)r));
    }
  }
  if (staUp) logged = 0;
}

}  // namespace

namespace net {

void begin(const String &hostname) {
  host = hostname;
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(host.c_str());
  WiFi.setAutoReconnect(true);
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);          // several APs, one SSID: strongest wins
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
  WiFi.onEvent(onWifiEvent);

  const Settings &s = settings::get();
  pHost = new WiFiManagerParameter("taHostIP",   "Tally Arbiter Server", s.taHost, sizeof(s.taHost) - 1);
  pPort = new WiFiManagerParameter("taHostPort", "Port",                 s.taPort, sizeof(s.taPort) - 1);
  wm.addParameter(pHost);
  wm.addParameter(pPort);
  wm.setSaveParamsCallback(onParamsSaved);
  wm.setSaveConfigCallback(onWifiSaved);
  std::vector<const char *> menu = { "wifi", "param", "info", "update", "sep", "restart", "exit" };
  wm.setMenu(menu);
  wm.setClass("invert");
  wm.setTitle("GP Tally v" FW_VERSION);
  wm.setHostname(host.c_str());
  wm.setAPStaticIPConfig(apAddress, apAddress, IPAddress(255, 255, 255, 0));
  wm.setConnectTimeout(15);
  wm.setConfigPortalBlocking(false);
  wm.setWebServerCallback([]() { wm.server->on("/erase", handleErase); });   // before WM's own

  setupOta();

  staDownSince = millis();
  if (wm.getWiFiIsSaved()) {
    LOGI("Connecting to saved WiFi");
    WiFi.begin();
  } else {
    LOGI("No saved WiFi");
    noCreds = true;
    openAp(false);
  }
}

void loop() {
  const uint32_t now = millis();
  logDisconnects();

  if (restartAt && (int32_t)(now - restartAt) >= 0) ESP.restart();

  // Setup AP — the library may close it by itself (save, Exit, timeout)
  const bool apNow = wm.getConfigPortalActive();
  if (apNow != apUp_) {
    apUp_ = apNow;
    changeCount++;
    LOGI("Setup AP %s", apNow ? "up" : "closed");
    if (!apNow) {
      if (!staUp) apSuppressed = true;   // closed without a network: stay closed
      apManual = false;
    }
  }

  // Station
  const bool up = WiFi.STA.connected() && WiFi.STA.hasIP();
  if (up != staUp) {
    staUp = up;
    changeCount++;
    if (up) {
      noCreds = apSuppressed = false;
      LOGI("Network connected: %s %s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
      if (!otaStarted) { ArduinoOTA.begin(); otaStarted = true; }
      if (apUp_ && !apManual) {
        wm.stopConfigPortal();
        LOGI("Network is back, closing setup AP");
      }
    } else {
      staDownSince = now;
      LOGI("Network connection lost");
    }
  }

  // Phone on the setup AP: leave the radio to the portal
  const bool pause = !staUp && apUp_ && WiFi.softAPgetStationNum() > 0;
  if (pause != retryPaused) {
    retryPaused = pause;
    WiFi.setAutoReconnect(!pause);
    if (pause) WiFi.disconnect();
    staRestartAt = 0;
    LOGI("%s", pause ? "Phone on setup AP: WiFi retries paused" : "Setup AP free: WiFi retries resumed");
  }

  if (!staUp) {
    const bool staOn = WiFi.getMode() & WIFI_MODE_STA;
    if (!noCreds && !retryPaused && !staOn && (staRestartAt == 0 || now - staRestartAt > WIFI_STA_RESTART_MS)) {
      staRestartAt = now;
      LOGI("Station was off, starting it again");
      WiFi.begin();
    }
    if (!apUp_ && !apSuppressed && (noCreds || now - staDownSince > WIFI_AP_AFTER_MS)) openAp(false);
  } else {
    tuneTxPower(now);
    ArduinoOTA.handle();
  }

  if (apUp_ || wm.getWebPortalActive()) wm.process();
}

bool     up()      { return staUp; }
bool     apUp()    { return apUp_; }
String   ssid()    { return WiFi.SSID(); }
String   ip()      { return WiFi.localIP().toString(); }
String   apIp()    { return apAddress.toString(); }
uint32_t changes() { return changeCount; }

void setWebPortal(bool on) { webPortal(on); }

void openSetupAp() {
  apSuppressed = false;
  openAp(true);
}

void closeSetupAp() {
  if (!wm.getConfigPortalActive()) return;
  LOGI("Setup AP closed by user");
  wm.stopConfigPortal();
}

bool eraseWifi() {
  WiFi.enableSTA(true);
  const uint32_t t = millis();
  while (!WiFi.STA.started() && millis() - t < 2000) delay(10);
  const bool ok = WiFi.eraseAP();
  if (ok) LOGI("WiFi config erased");
  else    LOGE("WiFi config erase failed");
  return ok;
}

}  // namespace net
