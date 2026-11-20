// ============================================================================
// board.h — Which StickC this is, and the listener identity derived from it.
// Detected at run time (M5Unified), so the code has no per-board #ifdefs.
// ============================================================================
#pragma once

#include <Arduino.h>

namespace board {

void begin();                     // after M5.begin()

const char   *model();            // "M5StickC-Plus"
const String &listenerName();     // "m5StickC-plus-5400ec": TA listener type, hostname, AP SSID
const char   *idSuffix();         // "5400ec"
int           ledProgramPin();
const char   *ledProgramPinName(); // "G0" / "G19"

}  // namespace board
