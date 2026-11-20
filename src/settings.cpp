#include "settings.h"

#include <Preferences.h>
#include "config.h"
#include "log.h"

namespace {

Settings s;

void putString(const char *key, const String &value) {
  Preferences p;
  p.begin(PREF_NAMESPACE, false);
  p.putString(key, value);
  p.end();
}

}  // namespace

namespace settings {

void load() {
  Preferences p;
  p.begin(PREF_NAMESPACE, true);
  String host = p.getString(PREF_TA_HOST, TA_DEFAULT_HOST);
  String port = p.getString(PREF_TA_PORT, TA_DEFAULT_PORT);
  s.deviceId   = p.getString(PREF_DEVICE_ID, "unassigned");
  s.deviceName = p.getString(PREF_DEVICE_NAME, "Unassigned");
  s.brightness = p.getInt(PREF_BRIGHTNESS, BRIGHTNESS_DEFAULT);
  p.end();

  if (host.isEmpty()) host = TA_DEFAULT_HOST;
  if (port.isEmpty()) port = TA_DEFAULT_PORT;
  host.toCharArray(s.taHost, sizeof(s.taHost));
  port.toCharArray(s.taPort, sizeof(s.taPort));
  s.brightness = constrain(s.brightness, BRIGHTNESS_MIN, BRIGHTNESS_MAX);

  LOGI("Settings: TA %s:%s, device '%s' (%s), brightness %d",
       s.taHost, s.taPort, s.deviceName.c_str(), s.deviceId.c_str(), s.brightness);
}

const Settings &get() { return s; }

void saveTaServer(const String &host, const String &port) {
  host.toCharArray(s.taHost, sizeof(s.taHost));
  port.toCharArray(s.taPort, sizeof(s.taPort));
  putString(PREF_TA_HOST, host);
  putString(PREF_TA_PORT, port);
}

void saveDevice(const String &id, const String &name) {
  if (id != s.deviceId)     { s.deviceId = id;     putString(PREF_DEVICE_ID, id); }
  if (name != s.deviceName) { s.deviceName = name; putString(PREF_DEVICE_NAME, name); }
}

void saveBrightness(int percent) {
  s.brightness = percent;
  Preferences p;
  p.begin(PREF_NAMESPACE, false);
  p.putInt(PREF_BRIGHTNESS, percent);
  p.end();
}

}  // namespace settings
