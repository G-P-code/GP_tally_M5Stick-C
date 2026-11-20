#include "board.h"

#include <M5Unified.h>
#include <esp_mac.h>
#include "config.h"

namespace {
const char *model_   = "M5StickC";
String      name_;
char        suffix_[7] = "";
}  // namespace

namespace board {

void begin() {
  const char *prefix = "m5StickC-";
  if (M5.getBoard() == m5::board_t::board_M5StickCPlus) {
    model_ = "M5StickC-Plus";
    prefix = "m5StickC-plus-";
  }
  // Last three bytes of the WiFi MAC: the listener identity TallyArbiter knows.
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(suffix_, sizeof(suffix_), "%02x%02x%02x", mac[3], mac[4], mac[5]);
  name_ = String(prefix) + suffix_;
}

const char   *model()            { return model_; }
const String &listenerName()     { return name_; }
const char   *idSuffix()         { return suffix_; }

}  // namespace board
