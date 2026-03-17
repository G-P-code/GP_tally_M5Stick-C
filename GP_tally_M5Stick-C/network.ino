// ============================================================================
// network.ino — WiFi connection, WifiManager portal, reconnection, TX power
// ============================================================================

WiFiManagerParameter *custom_taServer;
WiFiManagerParameter *custom_taPort;

void connectToNetwork() {
  WiFi.mode(WIFI_STA);
  WiFi.onEvent(WiFiEvent);
  logger("Connecting to SSID: " + String(WiFi.SSID()), "info");

  custom_taServer = new WiFiManagerParameter("taHostIP",   "Tally Arbiter Server", tallyarbiter_host, 40);
  custom_taPort   = new WiFiManagerParameter("taHostPort", "Port",                 tallyarbiter_port, 6);
  wm.addParameter(custom_taServer);
  wm.addParameter(custom_taPort);

  if (wm.getWiFiIsSaved()) {
    m5_println("connecting...");
  } else {
    m5_println("Configure on");
    m5_println("SSID: " + listenerDeviceName);
  }

  wm.setSaveParamsCallback(saveParamCallback);

  std::vector<const char *> menu = { "wifi", "param", "info", "sep", "restart", "exit" };
  wm.setMenu(menu);
  wm.setClass("invert");

  wm.setAPStaticIPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  // Cik ilgi gaida WiFi savienojumu pirms atver AP portālu (sekundēs).
  // Bez šī — gaida waitForConnectResult() kas var ilgt 60+ s.
  wm.setConnectTimeout(15);

  // Cik ilgi AP portāls ir atvērts bez aktivitātes pirms autoConnect() atgriež false.
  // 0 = nekad neaizver (gaida mūžīgi). Mēs izvēlamies gaida mūžīgi —
  // lietotājam jāspēj konfigurēt bez laika spiediena.
  wm.setConfigPortalTimeout(0);

  bool res = wm.autoConnect(listenerDeviceName.c_str());

  if (!res) {
    logger("Failed to connect to WiFi", "error");
    m5_println("WiFi timeout.");
    m5_println("Reboot to configure.");
    // Negaida mūžīgi — reboot pēc 10s lai lietotājs var mēģināt vēlreiz
    delay(10000);
    ESP.restart();
  } else {
    logger("WiFi connected :)", "info");
    networkConnected = true;
  }
}

String getParam(String name) {
  if (wm.server->hasArg(name)) return wm.server->arg(name);
  return "";
}

void saveParamCallback() {
  // Padarām redzamu serializētā logā pat bez debug režīma
  logger("[CALLBACK] saveParamCallback fired", "info");

  String str_taHost = getParam("taHostIP");
  String str_taPort = getParam("taHostPort");

  logger("New TA host: " + str_taHost + ":" + str_taPort, "info");

  if (str_taHost.length() == 0) {
    logger("saveParamCallback: empty host, ignoring", "error");
    return;
  }

  // Saglabā NVS
  preferences.begin(PREF_NAMESPACE, false);
  preferences.putString(PREF_TA_HOST, str_taHost);
  preferences.putString(PREF_TA_PORT, str_taPort);
  preferences.end();

  // Atjaunina runtime mainīgos
  str_taHost.toCharArray(tallyarbiter_host, 40);
  str_taPort.toCharArray(tallyarbiter_port, 6);

  logger("Params saved. Rebooting...", "info");

  // Restartē lai savienotos ar jaunajiem datiem no tīra sākuma.
  // Bez restart: old socket paliek, handleReconnection() savienojas
  // ar veco adresi, jaunā adrese netiek izmantota līdz nākamajam reboot.
  delay(500);  // lai web serveris var nosūtīt atbildi pārlūkam
  ESP.restart();
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case IP_EVENT_STA_GOT_IP:
      logger("Network connected!", "info");
      logger(WiFi.localIP().toString(), "info");
      networkConnected = true;
      reconnectDelay   = RECONNECT_BASE_DELAY;
      break;
    case WIFI_EVENT_STA_DISCONNECTED:
      logger("Network connection lost!", "info");
      networkConnected = false;
      break;
    default:
      break;
  }
}

// Reconnect to TallyArbiter with exponential backoff.
void handleReconnection() {
  if (taConnected || !networkConnected) return;
  unsigned long now = millis();
  if (now - lastReconnectAttempt < (unsigned long)reconnectDelay) return;
  lastReconnectAttempt = now;
  logger("Attempting reconnect (delay=" + String(reconnectDelay) + "ms)", "info");
  connectToServer();
  reconnectDelay = min(reconnectDelay * 2, RECONNECT_MAX_DELAY);
}

// Reduce TX power when signal is strong, checked every 10 s.
void optimizeWiFiPower() {
  if (!networkConnected) return;
  static unsigned long lastCheck = 0;
  unsigned long now = millis();
  if (now - lastCheck < 10000) return;
  lastCheck = now;
  if (WiFi.RSSI() > WIFI_POWER_SAVE_RSSI) {
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
  } else {
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
  }
}
