// ============================================================================
// settings.h — Everything kept in NVS
// ============================================================================
#pragma once

#include <Arduino.h>

struct Settings {
  char   taHost[40];
  char   taPort[6];
  String deviceId;     // TallyArbiter device this listener shows
  String deviceName;
  int    brightness;   // percent, BRIGHTNESS_MIN..BRIGHTNESS_MAX
};

namespace settings {

void            load();
const Settings &get();

void saveTaServer(const String &host, const String &port);
void saveDevice(const String &id, const String &name);
void saveBrightness(int percent);

}  // namespace settings
