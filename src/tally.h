// ============================================================================
// tally.h — TallyArbiter listener client (Socket.IO v4 over WebSockets)
// ============================================================================
#pragma once

#include <Arduino.h>

enum class TallyKind : uint8_t { None, Preview, Program, Aux, Other };

struct TallyState {
  TallyKind kind     = TallyKind::None;
  uint32_t  rgb      = 0;       // bus colour from TallyArbiter, 0xRRGGBB
  bool      hasColor = false;
  int       priority = 0;
};

namespace ta {

void begin(const String &listenerName);   // server and device from settings
void loop(bool networkUp);
void networkCameUp();                     // retry at once instead of waiting out the backoff

bool              connected();
const TallyState &state();
const String     &deviceName();
uint32_t          changes();              // bumps on anything the screen shows

bool takeFlash();                         // one-shot requests from the server
bool takeReassign();

const char *kindName(TallyKind kind);

}  // namespace ta
