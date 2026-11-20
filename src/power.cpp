#include "power.h"

#include <M5Unified.h>
#include "config.h"

namespace {

power::Battery batt;
uint32_t       changeCount = 0;
uint32_t       readAt      = 0;
bool           fast        = true;   // forces the first setCpuFrequencyMhz()

// USB present: VBUS where the PMIC reports it (AXP192), else the charge flag.
// "Charging" vs "charged" by level: the AXP192 charge current reads 0 well
// before the battery is full, so the level is the more reliable signal.
void read() {
  power::Battery b;
  b.level = constrain((int)M5.Power.getBatteryLevel(), 0, 100);
  const int  vbus = M5.Power.getVBUSVoltage();
  const bool usb  = vbus > 0 ? vbus > 3000 : M5.Power.isCharging();
  b.charging = usb && b.level < 95;
  b.charged  = usb && b.level >= 95;
  if (b.level != batt.level || b.charging != batt.charging || b.charged != batt.charged) {
    batt = b;
    changeCount++;
  }
}

}  // namespace

namespace power {

void begin() {
  btStop();
  loop(false);
  read();
}

void loop(bool busy) {
  if (busy != fast) {
    setCpuFrequencyMhz(busy ? 160 : 80);
    fast = busy;
  }
  const uint32_t now = millis();
  if (now - readAt >= BATTERY_READ_MS) {
    readAt = now;
    read();
  }
}

const Battery &battery() { return batt; }
uint32_t       changes() { return changeCount; }

}  // namespace power
