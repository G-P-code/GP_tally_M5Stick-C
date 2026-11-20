// ============================================================================
// net.h — WiFi, WiFiManager setup AP / web config, OTA
// ============================================================================
#pragma once

#include <Arduino.h>

namespace net {

void begin(const String &hostname);
void loop();

bool   up();                   // station has an IP
bool   apUp();                 // setup AP is open
String ssid();
String ip();
String apIp();
uint32_t changes();            // bumps when up() / apUp() change

void setWebPortal(bool on);    // config page on the station IP (Settings screen)
void openSetupAp();            // G33 short press; closes after SETUP_AP_MANUAL_TIMEOUT_S
void closeSetupAp();           // M5 long press / portal "Exit"
bool eraseWifi();              // forget the saved network

}  // namespace net
