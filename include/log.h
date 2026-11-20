// ============================================================================
// log.h — Serial logging. Build with -DVERBOSE for debug lines.
// ============================================================================
#pragma once

#include <Arduino.h>

#define LOGI(fmt, ...) Serial.printf("[INFO]  " fmt "\n", ##__VA_ARGS__)
#define LOGE(fmt, ...) Serial.printf("[ERROR] " fmt "\n", ##__VA_ARGS__)

#ifdef VERBOSE
  #define LOGD(fmt, ...) Serial.printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
  #define LOGD(...) do {} while (0)
#endif
