// ============================================================================
// GP Tally — TallyArbiter listener for M5StickC / StickC-Plus
//
// Based on the m5stickc-listener by Joseph Adams:
// https://github.com/josephdadams/TallyArbiter/tree/master/listener_clients/m5stickc-listener
//
// Modules:
//   board     which StickC this is, listener name
//   settings  NVS
//   net       WiFi, setup AP / web config, OTA
//   ta        TallyArbiter client -> tally state
//   leds      external tally LEDs
//   ui        display, buttons, animations
//   power     battery, CPU clock
// ============================================================================

#include <M5Unified.h>
#include "board.h"
#include "config.h"
#include "leds.h"
#include "log.h"
#include "net.h"
#include "power.h"
#include "settings.h"
#include "tally.h"
#include "ui.h"

namespace {

leds::Out ledOut(TallyKind kind) {
  switch (kind) {
    case TallyKind::Program: return leds::Out::Program;
    case TallyKind::Preview: return leds::Out::Preview;
    case TallyKind::Aux:     return leds::Out::Aux;
    default:                 return leds::Out::Off;
  }
}

bool netWasUp = false;

}  // namespace

void setup() {
  Serial.begin(115200);       // M5Unified 0.3 no longer starts it
  auto cfg = M5.config();
  cfg.internal_mic = false;   // G0 is the external Program LED on StickC / Plus
  cfg.internal_spk = false;
  M5.begin(cfg);

  board::begin();
  LOGI("GP Tally v" FW_VERSION " on %s, listener %s", board::model(), board::listenerName().c_str());

  settings::load();
  power::begin();
  leds::begin(PIN_LED_PROGRAM);
  ui::begin();
  net::begin(board::listenerName());
  ta::begin(board::listenerName());
}

void loop() {
  M5.update();

  net::loop();
  const bool netUp = net::up();
  if (netUp && !netWasUp) ta::networkCameUp();
  netWasUp = netUp;

  ta::loop(netUp);
  leds::show(ledOut(ta::state().kind));
  ui::loop();
  power::loop(ui::busy() || net::apUp() || !ta::connected());
}
