///////////////////////////////////////////////////////////////////////////////////
//    functions to modifiy, save, settings for connection to wifi and server      //
///////////////////////////////////////////////////////////////////////////////////

void saveConfigCallback () {
  shouldSaveConfig = true;
}

void quitConfigMode() {
  if ( configMode == 0 ) {
    return;
  }
  else if ( configMode == 1 ) {
    wifiManager.setTimeout(5);
    detachInterrupt(digitalPinToInterrupt(OTA_BUTTON_PIN));
    aSerial.vv().pln(F("Quit config mode"));
    return;
  }
}

void configModeCallback (WiFiManager *myWiFiManager) {
  aSerial.vv().pln(F("====== Config mode opening ======"));
  //aSerial.vvv().p(F("Portal SSID : ")).pln(myWiFiManager->getConfigPortalSSID());
  attachInterrupt(digitalPinToInterrupt(OTA_BUTTON_PIN), quitConfigMode, CHANGE);
  configMode = 1;
  wifiFailCount = 0;
  mqttFailCount = 0;
  ticker.attach(1, tick);
}

void initConfigManager(Config &config) {
#if DEBUG >= 3
  wifiManager.setDebugOutput(true);
#endif
#if DEBUG == 0
  wifiManager.setDebugOutput(false);
#endif
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 244, 1), IPAddress(192, 168, 244, 1), IPAddress(255, 255, 255, 0));
  wifiManager.addParameter(&customMqttServer);
  wifiManager.addParameter(&customMqttPort);
  wifiManager.addParameter(&customMqttUser);
  wifiManager.addParameter(&customMqttPassword);
  wifiManager.addParameter(&customCamResolution);
  wifiManager.addParameter(&customCamFpm);
  //  IPAddress _ip, _gw, _sn;
  //  _ip.fromString(config.staticIp);
  //  _gw.fromString(config.staticGw);
  //  _sn.fromString(config.staticSn);
  //  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
}

void configManager(Config &config) {
  //  wifiManager.addParameter(&customMqttServer);
  //  wifiManager.addParameter(&customMqttPort);
  //  wifiManager.addParameter(&customMqttUser);
  //  wifiManager.addParameter(&customMqttPassword);
  //  wifiManager.addParameter(&customCamResolution);
  //  wifiManager.addParameter(&customCamFpm);
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

  configCount++;
  // After first start, hard reset, or without any known WiFi AP
  aSerial.vv().p(F(" load config file.")).pln((const char*)config.mqttServer);

  //  if (WiFi.status() != WL_CONNECTED) {
  //    aSerial.vv().pln(F("Auto config access"));
  //    if (!wifiManager.autoConnect(config.devEui, config.devicePass)) {
  //      aSerial.v().pln(F("Connection failure --> Timeout"));
  //      delay(3000);
  //      setReboot();
  //    }
  //  }

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
  else if (WiFi.status() != WL_CONNECTED || !mqttClient.connected() || (strcmp(config.mqttServer, "") == 0) || (configCount > 0 && manualConfig == false)) {
    aSerial.vv().pln(F("User config access"));
    wifiManager.setTimeout(configTimeout);
    wifiManager.startConfigPortal(config.devEui, config.devicePass);
  }

  detachInterrupt(digitalPinToInterrupt(OTA_BUTTON_PIN));
  ticker.detach();
  digitalWrite(STATE_LED, HIGH);
  manualConfig = false;
  configMode = 0;
  if ( shouldSaveConfig ) {
    if ( (strcmp(customMqttServer.getValue(), "") != 0) ) {
      strlcpy(config.mqttServer, customMqttServer.getValue(), sizeof(config.mqttServer));
    }
    if ( (strcmp(customMqttPort.getValue(), "") != 0) ) {
      strlcpy(config.mqttPort, customMqttPort.getValue(), sizeof(config.mqttPort));
    }
    if ( (strcmp(customMqttUser.getValue(), "") != 0) ) {
      strlcpy(config.mqttUser, customMqttUser.getValue(), sizeof(config.mqttUser));
    }
    if ( (strcmp(customMqttPassword.getValue(), "") != 0) ) {
      strlcpy(config.mqttPassword, customMqttPassword.getValue(), sizeof(config.mqttPassword));
    }
    if ( (strcmp(customCamResolution.getValue(), "") != 0) ) {
      strlcpy(config.camResolution, customCamResolution.getValue(), sizeof(config.camResolution));
    }
    if ( (strcmp(customCamFpm.getValue(), "") != 0) ) {
      strlcpy(config.camFpm, customCamFpm.getValue(), sizeof(config.camFpm));
    }
    saveConfig(configFileName, config);
    loadRoutes(message, config);
  }
  aSerial.v().pln();
  aSerial.vvv().pln(F("Config successful")).p(F("Config mode counter :")).pln(configCount);
  aSerial.vvv().print(F("config heap size : ")).println(ESP.getFreeHeap());
  aSerial.vv().p(F("IP Address : ")).pln(WiFi.localIP());
  aSerial.v().pln(F("====== Config ended ======"));
}
