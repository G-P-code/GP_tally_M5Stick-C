#include "ui.h"

#include <M5Unified.h>
#include "board.h"
#include "config.h"
#include "leds.h"
#include "log.h"
#include "net.h"
#include "power.h"
#include "settings.h"
#include "tally.h"

namespace {

using Gfx = lgfx::LovyanGFX;

enum class Screen  : uint8_t { Tally, Settings };
enum class Overlay : uint8_t { None, Flash, Reassign, LedCheck, Reset };
enum class Trigger : uint8_t { Idle, Debounce, Holding };

M5Canvas canvas(&M5.Display);
bool     canvasOk = false;

// Screen choice: Settings while TallyArbiter is not connected, Tally once it
// is — unless the user picked a screen with the M5 button.
Screen screen       = Screen::Settings;
bool   userSettings = false;   // opened by hand: stays until closed by hand
bool   userTally    = false;   // tally picked by hand while TA is down
bool   taWasUp      = false;

Overlay  overlay   = Overlay::None;
int      step      = 0;
uint32_t stepAt    = 0;

bool     osd       = false;    // brightness bar
uint32_t osdUntil  = 0;

Trigger  trig      = Trigger::Idle;
uint32_t trigAt    = 0;

int      rotation  = 1;
uint32_t imuAt     = 0;

bool     dirty     = true;
uint32_t seenTa = 0, seenNet = 0, seenBatt = 0;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool small(Gfx &g) { return g.width() < 200; }   // StickC: 160x80

uint16_t rgb565(uint32_t rgb) {
  return M5.Display.color565(rgb >> 16, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

// A shade of the tally colour, pct % of the way to white (dark colour) or
// black (light colour): readable without glaring.
uint16_t shade(uint32_t rgb, int pct) {
  const int r = rgb >> 16, g = (rgb >> 8) & 0xFF, b = rgb & 0xFF;
  const int to = (r * 299 + g * 587 + b * 114) / 1000 < 128 ? 255 : 0;
  return M5.Display.color565(r + (to - r) * pct / 100, g + (to - g) * pct / 100, b + (to - b) * pct / 100);
}

const lgfx::IFont *textFont(Gfx &g) {
  return small(g) ? (const lgfx::IFont *)&fonts::Font0 : (const lgfx::IFont *)&fonts::FreeSans9pt7b;
}

void applyBrightness() {
  const int pct = settings::get().brightness;
  float norm = float(pct - BRIGHTNESS_MIN) / float(BRIGHTNESS_MAX - BRIGHTNESS_MIN);
  norm = constrain(norm, 0.0f, 1.0f);
  M5.Display.setBrightness(uint8_t(LCD_LEVEL_MIN + (255 - LCD_LEVEL_MIN) * norm));
  leds::setBrightness(pct);
}

void startOverlay(Overlay o) {
  overlay = o;
  step    = 0;
  stepAt  = millis();
  dirty   = true;
}

leds::Out ledCheckOut(int s) {
  static const leds::Out outs[LED_CHECK_STEPS] = { leds::Out::Program, leds::Out::Preview, leds::Out::Aux, leds::Out::All };
  return outs[s];
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

// Device name: the largest FreeSans that fits the width, centred a little
// above the middle (the device id sits in the bottom-right corner).
void drawName(Gfx &g, const String &name, uint16_t fg, uint16_t bg) {
  static const lgfx::IFont *const sizes[] = {
    &fonts::FreeSans24pt7b, &fonts::FreeSans18pt7b, &fonts::FreeSans12pt7b, &fonts::FreeSans9pt7b };
  for (const lgfx::IFont *f : sizes) {
    g.setFont(f);
    if (g.textWidth(name.c_str()) <= g.width() - 8) break;
  }
  g.setTextColor(fg, bg);
  g.setTextDatum(MC_DATUM);
  g.drawString(name.c_str(), g.width() / 2, g.height() / 2 - 4);
}

void renderTally(Gfx &g) {
  const TallyState &st = ta::state();
  const bool     active = st.kind != TallyKind::None && st.hasColor;
  const uint16_t bg     = active ? rgb565(st.rgb) : (uint16_t)TFT_BLACK;
  g.fillScreen(bg);
  drawName(g, ta::deviceName(), active ? shade(st.rgb, TALLY_NAME_CONTRAST) : COLOR_IDLE_NAME, bg);

  g.setFont(textFont(g));
  if (!leds::enabled()) {
    g.setTextColor(TFT_DARKGREY, bg);
    g.setTextDatum(TL_DATUM);
    g.drawString("LED OFF", 4, 4);
  }
  if (!osd) {
    g.setTextColor(active ? shade(st.rgb, TALLY_ID_CONTRAST) : COLOR_IDLE_ID, bg);
    g.setTextDatum(BR_DATUM);
    g.drawString(board::idSuffix(), g.width() - (small(g) ? 4 : 12), g.height() - (small(g) ? 2 : 6));
  }
}

void renderSettings(Gfx &g) {
  const bool sm = small(g);
  g.fillScreen(TFT_BLACK);
  g.setTextDatum(TL_DATUM);
  int y = sm ? 0 : 2;

  // One line; on the StickC a line too wide for the 16 px font drops to the
  // 8 px font instead of wrapping (a wrap pushes the last line off the screen).
  auto line = [&](const String &text, uint16_t color) {
    g.setFont(sm ? (const lgfx::IFont *)&fonts::Font2 : (const lgfx::IFont *)&fonts::FreeSans9pt7b);
    const int h = g.fontHeight();
    int dy = 0;
    if (sm && g.textWidth(text.c_str()) > g.width()) {
      g.setFont(&fonts::Font0);
      dy = (h - g.fontHeight()) / 2;
    }
    g.setTextColor(color, TFT_BLACK);
    g.drawString(text.c_str(), 0, y + dy);
    y += h;
  };

  if (net::apUp()) {
    line(String(sm ? "AP: " : "Setup AP: ") + board::listenerName(), TFT_YELLOW);
    line("http://" + net::apIp(), TFT_YELLOW);
  } else if (net::up()) {
    line("SSID: " + net::ssid(), TFT_WHITE);
    line(net::ip(), TFT_WHITE);
  } else {
    line("SSID: " + net::ssid(), TFT_WHITE);
    line("No WiFi - retrying...", TFT_RED);
  }

  const Settings &s = settings::get();
  const String server = String(s.taHost) + ":" + s.taPort;
  if (sm) {
    line("TA: " + server, TFT_WHITE);
  } else {
    line("Tally Arbiter Server:", TFT_WHITE);
    line(server, TFT_WHITE);
  }
  if (ta::connected()) line("Connected.", TFT_GREENYELLOW);
  else                 line("Not connected!", TFT_RED);

  const power::Battery &b = power::battery();
  const int batteryY = y;
  line(String("Battery: ") + (b.charging ? "Charging..." : b.charged ? "Charged." : String(b.level) + "%"), TFT_WHITE);
  g.setTextColor(TFT_DARKGREY, TFT_BLACK);
  g.setTextDatum(TR_DATUM);
  g.drawString("v" FW_VERSION, g.width() - (sm ? 2 : 12), batteryY);
}

void renderOsd(Gfx &g) {
  const int barH = 18, barW = g.width() - 8, barX = 4, barY = g.height() - barH - 4;
  g.fillRect(0, barY - 4, g.width(), barH + 8, TFT_BLACK);
  g.drawRect(barX, barY, barW, barH, TFT_DARKGREY);
  const int pct = settings::get().brightness;
  int filled = (pct - BRIGHTNESS_MIN) * barW / (BRIGHTNESS_MAX - BRIGHTNESS_MIN);
  g.fillRect(barX, barY, constrain(filled, 1, barW), barH, TFT_DARKGREY);
}

// Plain text lines on a coloured screen (LED check, reset prompt).
void renderLines(Gfx &g, uint16_t bg, uint16_t fg, std::initializer_list<String> lines) {
  g.fillScreen(bg);
  g.setFont(textFont(g));
  g.setTextColor(fg, bg);
  g.setTextDatum(TL_DATUM);
  int y = small(g) ? 0 : 24;
  for (const String &l : lines) {
    g.drawString(l.c_str(), small(g) ? 0 : 4, y);
    y += g.fontHeight();
  }
}

void renderLedCheck(Gfx &g) {
  const String prog = board::ledProgramPinName();
  struct { uint16_t bg, fg; const char *label; String pins; } s[LED_CHECK_STEPS] = {
    { TFT_RED,       TFT_WHITE,  "PROGRAM", prog },
    { TFT_DARKGREEN, TFT_WHITE,  "PREVIEW", "G26" },
    { TFT_NAVY,      TFT_YELLOW, "AUX",     "G25" },
    { TFT_DARKGREY,  TFT_WHITE,  "ALL ON",  prog + "+G26+G25" },
  };
  renderLines(g, s[step].bg, s[step].fg,
              { "-- LED CHECK --", "Step " + String(step + 1) + "/" + String(LED_CHECK_STEPS), "",
                s[step].label, "Pin: " + s[step].pins });
}

void render() {
  Gfx &g = canvasOk ? (Gfx &)canvas : (Gfx &)M5.Display;
  switch (overlay) {
    case Overlay::Flash:    g.fillScreen(step % 2 == 0 ? TFT_WHITE : TFT_BLACK); break;
    case Overlay::Reassign: g.fillScreen(step % 2 == 0 ? TFT_RED : TFT_BLACK);   break;
    case Overlay::LedCheck: renderLedCheck(g);                                   break;
    case Overlay::Reset:    renderLines(g, TFT_BLACK, TFT_WHITE, { "Reset button pushed...", "Hold 3 s: erase WiFi" }); break;
    case Overlay::None:
      if (screen == Screen::Tally) renderTally(g);
      else                         renderSettings(g);
      if (osd) renderOsd(g);
      break;
  }
  if (canvasOk) canvas.pushSprite(0, 0);
  dirty = false;
}

// A message shown right before a restart.
void renderFinal(const char *text) {
  overlay = Overlay::None;
  Gfx &g = canvasOk ? (Gfx &)canvas : (Gfx &)M5.Display;
  renderLines(g, TFT_BLACK, TFT_WHITE, { text });
  if (canvasOk) canvas.pushSprite(0, 0);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void brightnessClick(uint32_t now) {
  if (osd) {   // the first click only shows the bar
    int b = settings::get().brightness;
    b = b >= BRIGHTNESS_MAX ? BRIGHTNESS_MIN : min(b + BRIGHTNESS_STEP, BRIGHTNESS_MAX);
    settings::saveBrightness(b);
    applyBrightness();
    LOGD("Brightness %d", b);
  }
  osd      = true;
  osdUntil = now + OSD_MS;
  dirty    = true;
}

void handleButtons(uint32_t now) {
  auto &m5btn  = M5.BtnA;   // front
  auto &action = M5.BtnB;   // side

  if (m5btn.pressedFor(WIFI_ERASE_HOLD_MS)) {
    LOGI("M5 button held: erasing WiFi");
    renderFinal("Erasing WiFi...");
    net::eraseWifi();
    ESP.restart();
  }
  if (m5btn.wasHold() && net::apUp()) net::closeSetupAp();
  if (m5btn.wasSingleClicked()) {
    if (screen == Screen::Settings) { userSettings = false; userTally = !ta::connected(); }
    else                            { userSettings = true;  userTally = false; }
  }
  if (m5btn.wasDoubleClicked()) {
    leds::setEnabled(!leds::enabled());
    dirty = true;
  }

  if (action.wasClicked()) brightnessClick(now);
  if (action.wasHold()) {
    LOGI("LED check");
    startOverlay(Overlay::LedCheck);
    leds::test(ledCheckOut(0));
  }
}

// G33 on the grove port: short press = setup AP, hold = erase WiFi.
void handleTrigger(uint32_t now) {
  const bool low = digitalRead(PIN_TRIGGER) == LOW;
  switch (trig) {
    case Trigger::Idle:
      if (low) { trig = Trigger::Debounce; trigAt = now; }
      break;
    case Trigger::Debounce:
      if (now - trigAt < TRIGGER_DEBOUNCE_MS) break;
      if (!low) { trig = Trigger::Idle; break; }
      trig = Trigger::Holding;
      trigAt = now;
      LOGI("Reset button pressed");
      startOverlay(Overlay::Reset);
      break;
    case Trigger::Holding:
      if (low && now - trigAt >= TRIGGER_HOLD_MS) {
        renderFinal("Erasing WiFi...");
        net::eraseWifi();
        ESP.restart();
      } else if (!low) {
        trig    = Trigger::Idle;
        overlay = Overlay::None;
        dirty   = true;
        net::openSetupAp();
      }
      break;
  }
}

// Upside down: rotate the screen.
void handleRotation(uint32_t now) {
  if (now - imuAt < IMU_READ_MS) return;
  imuAt = now;
  float ax, ay, az;
  if (!M5.Imu.getAccel(&ax, &ay, &az)) return;
  int r = rotation;
  if (ax < -0.7f)     r = 3;
  else if (ax > 0.7f) r = 1;
  if (r == rotation) return;
  rotation = r;
  M5.Display.setRotation(r);
  dirty = true;
}

void advanceOverlay(uint32_t now) {
  uint32_t interval;
  int      steps;
  switch (overlay) {
    case Overlay::Flash:    interval = FLASH_STEP_MS;     steps = FLASH_STEPS;     break;
    case Overlay::Reassign: interval = REASSIGN_STEP_MS;  steps = REASSIGN_STEPS;  break;
    case Overlay::LedCheck: interval = LED_CHECK_STEP_MS; steps = LED_CHECK_STEPS; break;
    default: return;
  }
  if (now - stepAt < interval) return;
  stepAt = now;
  dirty  = true;
  if (++step < steps) {
    if (overlay == Overlay::LedCheck) leds::test(ledCheckOut(step));
    return;
  }
  if (overlay == Overlay::LedCheck) leds::endTest();
  overlay = Overlay::None;
}

}  // namespace

namespace ui {

void begin() {
  pinMode(PIN_TRIGGER, INPUT_PULLUP);
  M5.BtnA.setHoldThresh(BUTTON_HOLD_MS);
  M5.BtnB.setHoldThresh(BUTTON_HOLD_MS);
  M5.Display.setRotation(rotation);
  canvas.setColorDepth(16);
  canvasOk = canvas.createSprite(M5.Display.width(), M5.Display.height()) != nullptr;
  if (!canvasOk) LOGE("No memory for the screen buffer, drawing directly");
  applyBrightness();
  render();
}

void loop() {
  const uint32_t now = millis();

  handleButtons(now);
  handleTrigger(now);
  handleRotation(now);

  if (ta::takeFlash())    startOverlay(Overlay::Flash);
  if (ta::takeReassign()) startOverlay(Overlay::Reassign);
  advanceOverlay(now);

  if (osd && (int32_t)(now - osdUntil) >= 0) { osd = false; dirty = true; }

  // Which screen
  const bool taUp = ta::connected();
  if (taUp != taWasUp) { taWasUp = taUp; userTally = false; }
  const Screen want = userSettings ? Screen::Settings
                    : (taUp || userTally) ? Screen::Tally : Screen::Settings;
  if (want != screen) { screen = want; dirty = true; }
  net::setWebPortal(screen == Screen::Settings);

  // Data shown on screen
  if (ta::changes() != seenTa)       { seenTa = ta::changes();       dirty = true; }
  if (net::changes() != seenNet)     { seenNet = net::changes();     dirty = true; }
  if (power::changes() != seenBatt)  { seenBatt = power::changes();  dirty |= screen == Screen::Settings; }

  if (dirty) render();
}

bool busy() { return overlay != Overlay::None || osd; }

}  // namespace ui
