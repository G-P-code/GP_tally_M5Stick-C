// ============================================================================
// board.h — Board selection and M5Stack library includes
//
// Board defines (set by Arduino IDE board selector, or by boards/*.json in
// PlatformIO):
//   M5StickC       ARDUINO_M5STACK_STICKC
//   M5StickC-Plus  ARDUINO_M5STACK_STICKC_PLUS
//   M5StickC-Plus2 ARDUINO_M5STACK_STICKC_PLUS2
// ============================================================================
#pragma once

#if defined(ARDUINO_M5STACK_STICKC)
  #define STICK_C
#endif
#if defined(ARDUINO_M5STACK_STICKC_PLUS)
  #define STICK_C_PLUS
#endif
#if defined(ARDUINO_M5STACK_STICKC_PLUS2)
  #define STICK_C_PLUS2
#endif

// Manual override — uncomment exactly one if IDE auto-detection is unavailable:
//#define STICK_C
//#define STICK_C_PLUS
//#define STICK_C_PLUS2

// Default to M5StickC Plus if nothing is defined
#if !defined(STICK_C) && !defined(STICK_C_PLUS) && !defined(STICK_C_PLUS2)
  #define STICK_C_PLUS
#endif

// Guard against multiple defines
#if defined(STICK_C_PLUS2)
  #include <M5StickCPlus2.h>
  #if defined(STICK_C) || defined(STICK_C_PLUS)
    #error "Multiple m5stick board types defined"
  #endif
#endif
#if defined(STICK_C_PLUS)
  #include <M5StickCPlus.h>
  #if defined(STICK_C) || defined(STICK_C_PLUS2)
    #error "Multiple m5stick board types defined"
  #endif
#endif
#if defined(STICK_C)
  #include <M5StickC.h>
  #if defined(STICK_C_PLUS) || defined(STICK_C_PLUS2)
    #error "Multiple m5stick board types defined"
  #endif
#endif

#if !defined(STICK_C_PLUS2)
  #include "Free_Fonts.h"
#else
  // M5GFX (used by Plus2) includes FreeSans fonts via M5StickCPlus2.h.
  // Define the same aliases used elsewhere in the code.
  #define FSS9  &fonts::FreeSans9pt7b
  #define FSS24 &fonts::FreeSans24pt7b
#endif
