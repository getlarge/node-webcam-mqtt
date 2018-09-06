///////////////////////////////////////////////////////////////////////////////////
//    functions to modifiy, save, settings for connection to wifi and server      //
///////////////////////////////////////////////////////////////////////////////////

void saveConfigCallback () {
  shouldSaveConfig = true;
}

void configModeCallback (WiFiManager *myWiFiManager) {
  aSerial.v().pln(F("====== Config mode opening ========="));
  //aSerial.vvv().p(F("Portal SSID : ")).pln(myWiFiManager->getConfigPortalSSID());
  attachInterrupt(digitalPinToInterrupt(OTA_BUTTON_PIN), quitConfigMode, RISING);
  configMode = 1;
  wifiFailCount = 0;
  ticker.attach(1, tick);
}

void configManager() {
  configCount++;
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", config.mqtt_server, sizeof(config.mqtt_server));
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", config.mqtt_port, sizeof(config.mqtt_port));
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", config.mqtt_user, sizeof(config.mqtt_user));
  WiFiManagerParameter custom_mqtt_password("password", "mqtt password", config.mqtt_password, sizeof(config.mqtt_password));
  //WiFiManager wifiManager;

#if DEBUG >= 3
  wifiManager.setDebugOutput(true);
#endif
#if DEBUG == 0
  wifiManager.setDebugOutput(false);
#endif
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
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
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);

  // When manually asking ...
  if ((configCount > 0 && manualConfig == true)) {
    //manualConfig = false;
    aSerial.vv().pln(F("Manual config access"));
    wifiManager.setTimeout(configTimeout * 2);
    wifiManager.startConfigPortal(config.mqtt_client, devicePass);
  }

  // When wifi is already connected but connection got interrupted ...
  if ((configCount > 0  && config.mqtt_server == "" && manualConfig == false) || (configCount > 1 && mqttFailCount > 30 && manualConfig == false)) {
    aSerial.vv().pln(F("User config access"));
    wifiManager.setTimeout(configTimeout);
    wifiManager.startConfigPortal(config.mqtt_client, devicePass);
  }

  // After first start, hard reset, or without any known WiFi AP
  if (WiFi.status() != WL_CONNECTED) {
    aSerial.vv().pln(F("Auto config access"));
    if (!wifiManager.autoConnect(config.mqtt_client, devicePass)) {
      aSerial.v().pln(F("Connection failure --> Timeout"));
      delay(3000);
      setReboot();
    }
  }

  else {
    if (shouldSaveConfig && (strcmp(custom_mqtt_server.getValue(), "") != 0) ) {
      aSerial.vv().pln(F("Saving config"));
      strlcpy(config.mqtt_server, custom_mqtt_server.getValue(), sizeof(config.mqtt_server));
      strlcpy(config.mqtt_port, custom_mqtt_port.getValue(), sizeof(config.mqtt_port));
      strlcpy(config.mqtt_user, custom_mqtt_user.getValue(), sizeof(config.mqtt_user));
      strlcpy(config.mqtt_password, custom_mqtt_password.getValue(), sizeof(config.mqtt_password));
      strlcpy(config.mqtt_topic_in, config.mqtt_client, sizeof(config.mqtt_topic_in));
      strcat(config.mqtt_topic_in, message.in_prefix);
      strlcpy(config.mqtt_topic_out, config.mqtt_client, sizeof(config.mqtt_topic_out));
      strcat(config.mqtt_topic_out, message.out_prefix);
      //      StaticJsonDocument<objBufferSize> doc;
      //      JsonObject& obj = doc.to<JsonObject>();
      //StaticJsonBuffer<(objBufferSize)> doc;
      DynamicJsonBuffer doc; // JSON v5
      JsonObject& obj = doc.createObject();
      obj["mqtt_server"] = config.mqtt_server;
      obj["mqtt_port"] = config.mqtt_port;
      obj["mqtt_client"] = config.mqtt_client;
      obj["mqtt_user"] = config.mqtt_user;
      obj["mqtt_password"] = config.mqtt_password;
      obj["mqtt_topic_in"] = config.mqtt_topic_in;
      obj["mqtt_topic_out"] = config.mqtt_topic_out;
      File configFile = SPIFFS.open("/config.json", "w");
      if (!configFile) {
        aSerial.vv().pln(F("Failed to open config file"));
      }
      //      if (serializeJsonPretty(doc, Serial) == 0) {
      //        Serial.println(F("Failed to write to Serial"));
      //      }
      //      if (serializeJson(doc, configFile) == 0) {
      //        Serial.println(F("Failed to write to file"));
      //      }
      obj.printTo(configFile);
#if DEBUG != 0
      obj.prettyPrintTo(Serial);
#endif
      configFile.close();
    }
    ticker.detach();
    //detachInterrupt(OTA_BUTTON_PIN);
    digitalWrite(STATE_LED, HIGH);
    manualConfig = false;
    configMode = 0;
    aSerial.v().pln();
    aSerial.vvv().pln(F("Config successful")).p(F("Config mode counter :")).pln(configCount);
    aSerial.vvv().print(F("config heap size : ")).println(ESP.getFreeHeap());
    aSerial.vv().p(F("IP Address : ")).pln(WiFi.localIP());
    aSerial.v().pln(F("====== Config ended ========="));
  }

}
