///////////////////////////////////////////////////////////////////////////////////
//    functions to modifiy, save, settings for connection to wifi and server      //
///////////////////////////////////////////////////////////////////////////////////

void saveConfigCallback () {
  shouldSaveConfig = true;
}

void configModeCallback (WiFiManager *myWiFiManager) {
  aSerial.v().pln(F("====== Config mode opening ======"));
  //aSerial.vvv().p(F("Portal SSID : ")).pln(myWiFiManager->getConfigPortalSSID());
  //  attachInterrupt(digitalPinToInterrupt(OTA_BUTTON_PIN), quitConfigMode, CHANGE);
  configMode = 1;
  wifiFailCount = 0;
  ticker.attach(1, tick);
}

void configManager(Config &config) {
  configCount++;
  WiFiManagerParameter customMqttServer("server", "mqtt server", config.mqttServer, sizeof(config.mqttServer));
  WiFiManagerParameter customMqttPort("port", "mqtt port", config.mqttPort, sizeof(config.mqttPort));
  WiFiManagerParameter customMqttUser("user", "mqtt user", config.mqttUser, sizeof(config.mqttUser));
  WiFiManagerParameter customMqttPassword("password", "mqtt password", config.mqttPassword, sizeof(config.mqttPassword));
  WiFiManagerParameter customCamResolution("resolution", "cam resolution", config.camResolution, sizeof(config.camResolution));
  WiFiManagerParameter customCamFpm("fpm", "cam fpm", config.camFpm, sizeof(config.camFpm));

#if DEBUG >= 3
  wifiManager.setDebugOutput(true);
#endif
#if DEBUG == 0
  wifiManager.setDebugOutput(false);
#endif
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 244, 1), IPAddress(192, 168, 244, 1), IPAddress(255, 255, 255, 0));
  //  IPAddress _ip, _gw, _sn;
  //  _ip.fromString(static_ip);
  //  _gw.fromString(static_gw);
  //  _sn.fromString(static_sn);
  //  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  wifiManager.addParameter(&customMqttServer);
  wifiManager.addParameter(&customMqttPort);
  wifiManager.addParameter(&customMqttUser);
  wifiManager.addParameter(&customMqttPassword);
  wifiManager.addParameter(&customCamResolution);
  wifiManager.addParameter(&customCamFpm);

  //wifiManager.setBreakAfterConfig(true);
  wifiManager.setMinimumSignalQuality(10);
  String script;
  script += "<script>";
  script += "document.addEventListener('DOMContentLoaded', function() {";
  script +=     "var params = window.location.search.substring(1).split('&');";
  script +=     "for (var param of params) {";
  script +=         "param = param.split('=');";
  script +=         "try {";
  script +=             "document.getElementById( param[0] ).value = param[1];";
  script +=         "} catch (e) {";
  script +=             "console.log('WARNING param', param[0], 'not found in page');";
  script +=         "}";
  script +=     "}";
  script += "});";
  script += "</script>";
  wifiManager.setCustomHeadElement(script.c_str());

  // After first start, hard reset, or without any known WiFi AP
  if (WiFi.status() != WL_CONNECTED) {
    aSerial.vv().pln(F("Auto config access"));
    if (!wifiManager.autoConnect(config.devEui, config.devicePass)) {
      aSerial.v().pln(F("Connection failure --> Timeout"));
      delay(3000);
      setReboot();
    }
  }
  // When manually asking ...
  else if ((configCount > 0 && manualConfig == true)) {
    //manualConfig = false;
    aSerial.vv().pln(F("Manual config access"));
    wifiManager.setTimeout(configTimeout * 2);
    wifiManager.startConfigPortal(config.devEui, config.devicePass);
  }

  // When wifi is already connected but connection got interrupted ...
  else if ((config.mqttPort == "0") || (configCount > 0 && mqttFailCount >= mqttMaxFailedCount && manualConfig == false)) {
    aSerial.vv().pln(F("User config access"));
    wifiManager.setTimeout(configTimeout);
    wifiManager.startConfigPortal(config.devEui, config.devicePass);
  }

  if (shouldSaveConfig && (strcmp(customMqttServer.getValue(), "") != 0) ) {
    aSerial.vv().pln(F("Saving config"));
    strlcpy(config.mqttServer, customMqttServer.getValue(), sizeof(config.mqttServer));
    strlcpy(config.mqttPort, customMqttPort.getValue(), sizeof(config.mqttPort));
    strlcpy(config.mqttUser, customMqttUser.getValue(), sizeof(config.mqttUser));
    strlcpy(config.mqttPassword, customMqttPassword.getValue(), sizeof(config.mqttPassword));
    strlcpy(config.camResolution, customCamResolution.getValue(), sizeof(config.camResolution));
    strlcpy(config.camFpm, customCamFpm.getValue(), sizeof(config.camFpm));
    saveConfig(configFileName, config);
  }
  ticker.detach();
  //  detachInterrupt(OTA_BUTTON_PIN);
  digitalWrite(STATE_LED, HIGH);
  manualConfig = false;
  configMode = 0;
  aSerial.v().pln();
  aSerial.vvv().pln(F("Config successful")).p(F("Config mode counter :")).pln(configCount);
  aSerial.vvv().print(F("config heap size : ")).println(ESP.getFreeHeap());
  aSerial.vv().p(F("IP Address : ")).pln(WiFi.localIP());
  aSerial.v().pln(F("====== Config ended ======"));
}
