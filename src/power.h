// ============================================================================
// power.h — Battery state and CPU clock
// ============================================================================
#pragma once

#include <stdint.h>

namespace power {

struct Battery {
  int  level    = 0;      // percent
  bool charging = false;  // on USB, not full yet
  bool charged  = false;  // on USB and full
};

void begin();
void loop(bool busy);           // busy: 160 MHz, otherwise 80 MHz

const Battery &battery();
uint32_t       changes();       // bumps when the battery state changes

}  // namespace power
