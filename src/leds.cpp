#include "leds.h"

#include <Arduino.h>
#include "config.h"
#include "log.h"

namespace {

int       pinProgram = -1;
int       duty       = 0;
bool      enabled_   = true;
bool      testing    = false;
leds::Out tally      = leds::Out::Off;
leds::Out testOut    = leds::Out::Off;

void write(leds::Out out) {
  if (pinProgram < 0) return;
  int program = 0, preview = 0, aux = 0;
  switch (out) {
    case leds::Out::Program: program = duty;                              break;
    case leds::Out::Preview: preview = duty;                              break;
    case leds::Out::Aux:     aux = duty / 4;                              break;   // aux LED is brighter
    case leds::Out::All:     program = duty; preview = duty; aux = duty / 4; break;
    case leds::Out::Off:                                                  break;
  }
  ledcWrite(pinProgram, program);
  ledcWrite(PIN_LED_PREVIEW, preview);
  ledcWrite(PIN_LED_AUX, aux);
}

void apply() { write(testing ? testOut : (enabled_ ? tally : leds::Out::Off)); }

const char *name(leds::Out out) {
  switch (out) {
    case leds::Out::Program: return "program";
    case leds::Out::Preview: return "preview";
    case leds::Out::Aux:     return "aux";
    case leds::Out::All:     return "all";
    default:                 return "off";
  }
}

}  // namespace

namespace leds {

void begin(int programPin) {
  pinProgram = programPin;
  ledcAttach(pinProgram, LED_PWM_FREQ, LED_PWM_BITS);
  ledcAttach(PIN_LED_PREVIEW, LED_PWM_FREQ, LED_PWM_BITS);
  ledcAttach(PIN_LED_AUX, LED_PWM_FREQ, LED_PWM_BITS);
  apply();
}

// Quadratic curve: equal brightness steps look equally big.
void setBrightness(int percent) {
  float norm = float(percent - BRIGHTNESS_MIN) / float(BRIGHTNESS_MAX - BRIGHTNESS_MIN);
  norm = constrain(norm, 0.0f, 1.0f);
  duty = int(LED_MIN_DUTY + (LED_MAX_DUTY - LED_MIN_DUTY) * norm * norm);
  apply();
}

void show(Out out) {
  if (out == tally) return;
  tally = out;
  LOGI("LEDs: %s", name(out));
  apply();
}

void setEnabled(bool on) {
  enabled_ = on;
  LOGI("LEDs %s", on ? "enabled" : "disabled");
  apply();
}

bool enabled() { return enabled_; }

void test(Out out) { testing = true; testOut = out; apply(); }
void endTest()     { testing = false; apply(); }

}  // namespace leds
