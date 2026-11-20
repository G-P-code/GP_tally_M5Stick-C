// ============================================================================
// ui.h — Display and controls
//
// Screens: Tally, Settings. On top of them, one overlay at a time (flash,
// reassign, LED check, G33 reset prompt) and the brightness bar.
// Whatever changes only marks the frame dirty; render() draws the whole frame
// from the current state into an off-screen canvas and pushes it at once.
// ============================================================================
#pragma once

namespace ui {

void begin();
void loop();
bool busy();     // an animation or the brightness bar is showing

}  // namespace ui
