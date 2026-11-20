// ============================================================================
// leds.h — External tally LEDs (Program / Preview / Aux, PWM)
// ============================================================================
#pragma once

#include <stdint.h>

namespace leds {

enum class Out : uint8_t { Off, Program, Preview, Aux, All };

void begin(int programPin);
void setBrightness(int percent);   // BRIGHTNESS_MIN..MAX, perceptual curve

void show(Out out);                // the tally output
void setEnabled(bool on);          // user switch (M5 double click)
bool enabled();

void test(Out out);                // LED check: force a pattern
void endTest();                    // back to the tally output

}  // namespace leds
