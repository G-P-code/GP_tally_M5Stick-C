// ============================================================================
// tally.cpp — TallyArbiter listener client
//
// Socket.IO v4 directly over WebSocketsClient (links2004/WebSockets):
//   Engine.IO packet (first char): '0' open (ping timing), '2' ping -> reply '3',
//                                  '4' message
//   Socket.IO packet (second char): '0' connect, '2' event ["name", ...args]
//
// The tally is recomputed from three pieces the server sends in any order:
//   bus_options    bus id -> type, colour, priority
//   device_states  which buses have sources on which device
//   devices        device id -> name
// The active bus with the highest priority wins.
// ============================================================================

#include "tally.h"

#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <vector>
#include "config.h"
#include "log.h"
#include "settings.h"

namespace {

struct Bus {
  String    id;
  TallyKind kind;
  uint32_t  rgb;
  bool      hasColor;
  int       priority;
};

struct Device {
  String id;
  String name;
};

WebSocketsClient    ws;
String              listener;

std::vector<Bus>    buses;
std::vector<String> activeBuses;   // buses with sources on our device
std::vector<Device> devices;

TallyState st;
String     name;
bool       connected_  = false;
uint32_t   changeCount = 0;
bool       flashReq    = false;
bool       reassignReq = false;

// Liveness: the server pings every pingInterval; nothing for
// pingInterval + pingTimeout means the link is dead even if TCP looks open.
uint32_t lastRxMs       = 0;
uint32_t pingIntervalMs = TA_PING_INTERVAL_MS;
uint32_t pingTimeoutMs  = TA_PING_TIMEOUT_MS;

// Backoff: a connect attempt can block up to 5 s while the server is away,
// so the library's retry interval widens the longer TA stays unreachable.
uint32_t retryMs      = TA_RETRY_MIN_MS;
uint32_t retryCheckAt = 0;

TallyKind kindOf(const char *type) {
  if (!type || !*type)            return TallyKind::None;
  if (!strcmp(type, "program"))   return TallyKind::Program;
  if (!strcmp(type, "preview"))   return TallyKind::Preview;
  if (!strcmp(type, "aux"))       return TallyKind::Aux;
  return TallyKind::Other;
}

// "#rrggbb" -> 0xRRGGBB
bool parseColor(const char *s, uint32_t &rgb) {
  if (!s) return false;
  if (*s == '#') s++;
  if (strlen(s) != 6) return false;
  for (int i = 0; i < 6; i++) if (!isxdigit((unsigned char)s[i])) return false;
  rgb = strtoul(s, nullptr, 16);
  return true;
}

void setReconnectInterval(uint32_t ms) {
  retryMs = ms;
  retryCheckAt = millis();
  ws.setReconnectInterval(ms);
}

void emit(const char *event, JsonVariantConst payload = JsonVariantConst()) {
  JsonDocument doc;
  JsonArray msg = doc.to<JsonArray>();
  msg.add(event);
  if (!payload.isNull()) msg.add(payload);
  String body;
  serializeJson(doc, body);
  String frame = "42" + body;
  ws.sendTXT(frame);
}

void recompute() {
  TallyState next;
  bool found = false;
  for (const String &id : activeBuses) {
    for (const Bus &b : buses) {
      if (b.id != id) continue;
      if (!found || b.priority > next.priority) {
        next  = { b.kind, b.rgb, b.hasColor, b.priority };
        found = true;
      }
    }
  }
  if (next.kind == st.kind && next.rgb == st.rgb && next.hasColor == st.hasColor) return;
  st = next;
  changeCount++;
  LOGI("Tally: %s (#%06lx, priority %d)", ta::kindName(st.kind), (unsigned long)st.rgb, st.priority);
}

void resolveName() {
  const String &id = settings::get().deviceId;
  for (const Device &d : devices) {
    if (d.id != id) continue;
    if (d.name != name) {
      name = d.name;
      changeCount++;
      LOGI("Device: %s", name.c_str());
    }
    break;
  }
  settings::saveDevice(id, name);   // writes only what changed
}

void setDeviceId(const String &id) {
  if (id == settings::get().deviceId) return;
  settings::saveDevice(id, name);
  activeBuses.clear();               // they belonged to the old device
  recompute();
  resolveName();
}

// ---------------------------------------------------------------------------
// Socket.IO
// ---------------------------------------------------------------------------

void onOpen(const char *json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return;
  pingIntervalMs = doc["pingInterval"] | TA_PING_INTERVAL_MS;
  pingTimeoutMs  = doc["pingTimeout"]  | TA_PING_TIMEOUT_MS;
}

void onConnect() {
  LOGI("Connected to TallyArbiter");
  JsonDocument p;
  p["deviceId"]        = settings::get().deviceId;
  p["listenerType"]    = listener;
  p["canBeReassigned"] = true;
  p["canBeFlashed"]    = true;
  p["supportsChat"]    = true;
  emit("listenerclient_connect", p.as<JsonVariantConst>());

  connected_ = true;
  lastRxMs   = millis();
  setReconnectInterval(TA_RETRY_MIN_MS);
  changeCount++;
}

void onDisconnect() {
  if (connected_) LOGI("Disconnected from TallyArbiter");
  connected_ = false;
  buses.clear();
  activeBuses.clear();
  recompute();
  retryCheckAt = millis();
  changeCount++;
}

void onReassign(const String &oldId, const String &newId) {
  LOGI("Reassign: %s -> %s", oldId.c_str(), newId.c_str());
  JsonDocument p;
  p["oldDeviceId"] = oldId;
  p["newDeviceId"] = newId;
  emit("listener_reassign_object", p.as<JsonVariantConst>());
  emit("devices");
  setDeviceId(newId);
  reassignReq = true;
}

void onEvent(const char *json, size_t len) {
  JsonDocument doc;
  if (DeserializationError err = deserializeJson(doc, json, len)) {
    LOGE("TallyArbiter JSON: %s", err.c_str());
    return;
  }
  JsonArrayConst msg = doc.as<JsonArrayConst>();
  const char *event = msg[0];
  if (!event) return;
  LOGD("Event %s", event);

  if (!strcmp(event, "bus_options")) {
    buses.clear();
    for (JsonObjectConst b : msg[1].as<JsonArrayConst>()) {
      Bus bus;
      bus.id       = b["id"].as<String>();
      bus.kind     = kindOf(b["type"]);
      bus.rgb      = 0;
      bus.hasColor = parseColor(b["color"], bus.rgb);
      bus.priority = b["priority"] | 0;
      buses.push_back(bus);
    }
    recompute();

  } else if (!strcmp(event, "device_states")) {
    const String &id = settings::get().deviceId;
    activeBuses.clear();
    for (JsonObjectConst s : msg[1].as<JsonArrayConst>()) {
      if (s["deviceId"].as<String>() != id) continue;
      if (s["sources"].as<JsonArrayConst>().size() == 0) continue;
      activeBuses.push_back(s["busId"].as<String>());
    }
    recompute();

  } else if (!strcmp(event, "devices")) {
    devices.clear();
    for (JsonObjectConst d : msg[1].as<JsonArrayConst>())
      devices.push_back({ d["id"].as<String>(), d["name"].as<String>() });
    resolveName();

  } else if (!strcmp(event, "deviceId")) {
    setDeviceId(msg[1].as<String>());

  } else if (!strcmp(event, "reassign")) {
    onReassign(msg[1].as<String>(), msg[2].as<String>());

  } else if (!strcmp(event, "flash")) {
    flashReq = true;

  } else if (!strcmp(event, "messaging")) {
    // "server" messages are TallyArbiter's own notices (e.g. our previous
    // session closing); chat from an operator is worth a line in the log.
    const char *type = msg[1] | "";
    if (!strcmp(type, "server")) LOGD("Server message: %s", msg[3] | "");
    else                         LOGI("Message (%s): %s", type, msg[3] | "");
  }
}

void wsEvent(WStype_t type, uint8_t *payload, size_t len) {
  switch (type) {
    case WStype_DISCONNECTED:
      onDisconnect();
      break;
    case WStype_CONNECTED:
      ws.sendTXT("40");                       // Socket.IO connect
      break;
    case WStype_TEXT: {
      if (len == 0) break;
      lastRxMs = millis();
      const char *m = (const char *)payload;
      if (m[0] == '0')      onOpen(m + 1);
      else if (m[0] == '2') ws.sendTXT("3");  // pong
      else if (m[0] == '4' && len > 1) {
        if (m[1] == '0')      onConnect();
        else if (m[1] == '2') onEvent(m + 2, len - 2);
      }
      break;
    }
    case WStype_ERROR:
      LOGE("WebSocket error");
      break;
    default:
      break;
  }
}

}  // namespace

namespace ta {

void begin(const String &listenerName) {
  listener = listenerName;
  name     = settings::get().deviceName;
  const Settings &s = settings::get();
  if (!strcmp(s.taHost, TA_DEFAULT_HOST)) {
    LOGI("TallyArbiter server not set (portal: Setup)");
    return;
  }
  LOGI("TallyArbiter: %s:%s", s.taHost, s.taPort);
  ws.onEvent(wsEvent);
  setReconnectInterval(TA_RETRY_MIN_MS);
  ws.begin(s.taHost, atoi(s.taPort), "/socket.io/?EIO=4&transport=websocket");
}

void loop(bool networkUp) {
  ws.loop();
  const uint32_t now = millis();

  if (connected_ && now - lastRxMs > pingIntervalMs + pingTimeoutMs + TA_PING_MARGIN_MS) {
    LOGE("No ping from TallyArbiter for %lu s, disconnecting", (unsigned long)((now - lastRxMs) / 1000));
    ws.disconnect();
    if (connected_) onDisconnect();   // in case the library raised no event
    return;
  }

  if (connected_ || !networkUp) return;
  if (now - retryCheckAt < retryMs) return;
  setReconnectInterval(min(retryMs * 2, TA_RETRY_MAX_MS));
  LOGD("TallyArbiter unreachable, next retry in %lu ms", (unsigned long)retryMs);
}

void networkCameUp() { setReconnectInterval(TA_RETRY_MIN_MS); }

bool              connected()  { return connected_; }
const TallyState &state()      { return st; }
const String     &deviceName() { return name; }
uint32_t          changes()    { return changeCount; }

bool takeFlash()    { bool r = flashReq;    flashReq = false;    return r; }
bool takeReassign() { bool r = reassignReq; reassignReq = false; return r; }

const char *kindName(TallyKind kind) {
  switch (kind) {
    case TallyKind::Program: return "program";
    case TallyKind::Preview: return "preview";
    case TallyKind::Aux:     return "aux";
    case TallyKind::Other:   return "other";
    default:                 return "none";
  }
}

}  // namespace ta
