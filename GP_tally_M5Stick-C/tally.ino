// ============================================================================
// tally.ino — TallyArbiter socket communication and tally data processing
// ============================================================================

void connectToServer() {
  logger("Connecting to Tally Arbiter host: " + String(tallyarbiter_host), "info");
  socket.onEvent(socket_event);
  socket.begin(tallyarbiter_host, atol(tallyarbiter_port));
}

void ws_emit(String event, const char *payload) {
  if (payload) {
    socket.sendEVENT("[\"" + event + "\"," + payload + "]");
  } else {
    socket.sendEVENT("[\"" + event + "\"]");
  }
}

// ----------------------------------------------------------------------------
// Main socket event dispatcher
// ----------------------------------------------------------------------------

void socket_event(socketIOmessageType_t type, uint8_t *payload, size_t length) {
  String eventMsg     = "";
  String eventType    = "";
  String eventContent = "";

  switch (type) {
    case sIOtype_CONNECT:
      socket_Connected((char *)payload, length);
      taConnected    = true;
      everConnected  = true;
      reconnectDelay = RECONNECT_BASE_DELAY;
      if (currentScreen == 1 && !userRequestedSettings) {
        switchToTally();
      } else if (currentScreen == 1) {
        paintSettingsScreen();  // refresh connection status line
      }
      break;

    case sIOtype_DISCONNECT:
      taConnected    = false;
      actualType     = "";
      actualColor    = "";
      actualPriority = 0;
      prevType       = "__invalidated__";
      prevLedType    = "__invalidated__";
#if TALLY_EXTRA_OUTPUT
      externalLed();           // turn all LEDs off immediately
      prevLedType = actualType;
#endif
      if (!everConnected) {
        // Never connected yet — already on settings from startup.
        // Just refresh the status line, no screen switch needed.
        paintSettingsScreen();
      } else {
        // Real disconnect after a successful connection.
        if (currentScreen == 0) {
          switchToSettings();  // byUser=false — reconnect will undo this
        } else {
          paintSettingsScreen();
        }
      }
      break;

    case sIOtype_ACK:
    case sIOtype_ERROR:
    case sIOtype_BINARY_EVENT:
    case sIOtype_BINARY_ACK:
      break;

    case sIOtype_EVENT:
      eventMsg     = (char *)payload;
      eventType    = eventMsg.substring(2, eventMsg.indexOf("\"", 2));
      eventContent = eventMsg.substring(eventType.length() + 4);
      eventContent.remove(eventContent.length() - 1);

      logger("Got event '" + eventType + "'", "info-quiet");

      if (eventType == "bus_options") {
        BusOptions = JSON.parse(eventContent);
        busOptionsReady = true;
        // Process any device_states that arrived before bus_options
        if (DeviceStates.length() > 0) processTallyData();
      }

      if (eventType == "reassign")  socket_Reassign(eventContent);
      if (eventType == "flash")     socket_Flash();
      if (eventType == "messaging") socket_Messaging(eventContent);

      if (eventType == "deviceId") {
        DeviceId = eventContent.substring(1, eventContent.length() - 1);
        SetDeviceName();
        if (!userRequestedSettings) switchToTally();
      }

      if (eventType == "devices") {
        Devices = JSON.parse(eventContent);
        SetDeviceName();
      }

      if (eventType == "device_states") {
        DeviceStates = JSON.parse(eventContent);
        if (busOptionsReady) processTallyData();
      }
      break;

    default:
      break;
  }
}

void socket_Connected(const char *payload, size_t length) {
  logger("Connected to Tally Arbiter server.", "info");
  logger("DeviceId: " + DeviceId, "info-quiet");
  char deviceObj[256];
  snprintf(deviceObj, sizeof(deviceObj),
    "{\"deviceId\": \"%s\", \"listenerType\": \"%s\", "
    "\"canBeReassigned\": true, \"canBeFlashed\": true, \"supportsChat\": true }",
    DeviceId.c_str(), listenerDeviceName.c_str());
  ws_emit("listenerclient_connect", deviceObj);
}

// ----------------------------------------------------------------------------
// Flash animation trigger
// ----------------------------------------------------------------------------

void socket_Flash() {
  flashActive = true;
  flashStep   = 0;
  flashLastMs = 0;
}

// ----------------------------------------------------------------------------
// Reassign
// ----------------------------------------------------------------------------

String strip_quot(String str) {
  if (str[0] == '"')         str.remove(0, 1);
  if (str.endsWith("\""))    str.remove(str.length() - 1, 1);
  return str;
}

void socket_Reassign(String payload) {
  logger("socket_Reassign()", "info-quiet");
  String oldDeviceId = payload.substring(0, payload.indexOf(','));
  String newDeviceId = payload.substring(oldDeviceId.length() + 1);
  newDeviceId = newDeviceId.substring(0, newDeviceId.indexOf(','));
  oldDeviceId = strip_quot(oldDeviceId);
  newDeviceId = strip_quot(newDeviceId);

  char charReassignObj[256];
  snprintf(charReassignObj, sizeof(charReassignObj),
    "{\"oldDeviceId\": \"%s\", \"newDeviceId\": \"%s\"}",
    oldDeviceId.c_str(), newDeviceId.c_str());
  ws_emit("listener_reassign_object", charReassignObj);
  ws_emit("devices");

  DeviceId = newDeviceId;
  preferences.begin(PREF_NAMESPACE, false);
  preferences.putString(PREF_DEVICE_ID, newDeviceId);
  preferences.end();

  // Clear tally state immediately — new device state not yet known.
  // This happens only once at reassign, not on every SetDeviceName() call.
  actualType     = "";
  actualColor    = "";
  actualPriority = 0;
  prevType       = "__invalidated__";
  prevLedType    = "__invalidated__";
#if TALLY_EXTRA_OUTPUT
  externalLed();
  prevLedType = actualType;
#endif

  SetDeviceName();

  reassignActive = true;
  reassignStep   = 0;
  reassignLastMs = 0;
}

void socket_Messaging(String payload) {
  int typeIdx    = payload.indexOf(',');
  String msgType = payload.substring(0, typeIdx);
  msgType.replace("\"", "");
  int msgIdx     = payload.lastIndexOf(',');
  String msg     = payload.substring(msgIdx + 1);
  msg.replace("\"", "");
  LastMessage = msgType + ": " + msg;
  evaluateMode();
}

// ----------------------------------------------------------------------------
// Tally data processing
// ----------------------------------------------------------------------------

void processTallyData() {
  bool   typeChanged    = false;
  int    highestPriority = -1;
  String highestType    = "";
  String highestColor   = "";
  int    highestPrioVal = 0;

  for (int i = 0; i < DeviceStates.length(); i++) {
    // Only process states that belong to our assigned device
    if (JSON.stringify(DeviceStates[i]["deviceId"]) != "\"" + DeviceId + "\"") continue;
    if (DeviceStates[i]["sources"].length() > 0) {
      typeChanged = true;
      String busType  = getBusTypeById(JSON.stringify(DeviceStates[i]["busId"]));
      String busColor = getBusColorById(JSON.stringify(DeviceStates[i]["busId"]));
      int    busPrio  = getBusPriorityById(JSON.stringify(DeviceStates[i]["busId"]));
      if (busPrio > highestPriority) {
        highestPriority = busPrio;
        highestType     = busType;
        highestColor    = busColor;
        highestPrioVal  = busPrio;
      }
    }
  }

  if (typeChanged) {
    actualType     = highestType;
    actualColor    = highestColor;
    actualPriority = highestPrioVal;
  } else {
    actualType     = "";
    actualColor    = "";
    actualPriority = 0;
  }

  // Auto-return to tally screen on active signal unless user opened settings manually
  if (typeChanged && currentScreen == 1 && !userRequestedSettings) {
    switchToTally();
    return;
  }

  evaluateMode();
}

String getBusTypeById(String busId) {
  for (int i = 0; i < BusOptions.length(); i++)
    if (JSON.stringify(BusOptions[i]["id"]) == busId)
      return JSON.stringify(BusOptions[i]["type"]);
  return "invalid";
}

String getBusColorById(String busId) {
  for (int i = 0; i < BusOptions.length(); i++)
    if (JSON.stringify(BusOptions[i]["id"]) == busId)
      return JSON.stringify(BusOptions[i]["color"]);
  return "invalid";
}

int getBusPriorityById(String busId) {
  for (int i = 0; i < BusOptions.length(); i++)
    if (JSON.stringify(BusOptions[i]["id"]) == busId)
      return JSON.stringify(BusOptions[i]["priority"]).toInt();
  return 0;
}

void SetDeviceName() {
  for (int i = 0; i < Devices.length(); i++) {
    if (JSON.stringify(Devices[i]["id"]) == "\"" + DeviceId + "\"") {
      String strDevice = JSON.stringify(Devices[i]["name"]);
      DeviceName = strDevice.substring(1, strDevice.length() - 1);
      break;
    }
  }
  preferences.begin(PREF_NAMESPACE, false);
  preferences.putString(PREF_DEVICE_NAME, DeviceName);
  preferences.end();
  logger("DeviceName: " + DeviceName, "info");
  // Invalidate display so the new name is repainted with the current tally state.
  // Do NOT clear actualType/Color here — this function is called both on reassign
  // AND on normal 'devices' event refresh. Clearing state on a normal refresh
  // would wipe a valid tally signal. State is cleared in socket_Reassign() instead.
  prevType = "__invalidated__";
  evaluateMode();
}
