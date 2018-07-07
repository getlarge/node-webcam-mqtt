///////////////////////////////////////////////////////////////////////////////////
//    functions to modifiy, save, settings for connection to wifi and server      //
///////////////////////////////////////////////////////////////////////////////////

void saveConfigCallback () {
  Serial.println(F("Should save config"));
  shouldSaveConfig = true;
}

void configModeCallback (WiFiManager *myWiFiManager) {
 // delay(1000);
 // check mqtt connection ?
  Serial.println(F("Entered config mode"));
  //  if((WiFi.status() == WL_CONNECTED)) {
}

void configManager() {
  ticker.detach();
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", config.mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", config.mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", config.mqtt_user, 20);
  WiFiManagerParameter custom_mqtt_password("password", "mqtt password", config.mqtt_password, 30);

  Serial.println(F("====== Config mode opening ========="));
  checkButton(1);
  ticker.attach(1, tick);
  configCount++;
  
  WiFiManager wifiManager;
  #if DEBUG_WM == 1 
    wifiManager.setDebugOutput(true);
  #endif
  #if DEBUG_WM == 0 
    wifiManager.setDebugOutput(false);
  #endif
  wifiManager.setAPCallback(configModeCallback);
//  wifiManager.setBreakAfterConfig(true);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
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


  configMode = 1;
  wifiFailCount = 0;

 // When no credentials or asking ...
  if (( !(const char*)config.mqtt_server ) || (configCount > 1 && manualConfig == true)) {
    Serial.println(F("Manual config access"));
    wifiManager.setConfigPortalTimeout(configTimeout1);
    wifiManager.startConfigPortal(config.mqtt_client, devicePass);
    //wifiManager.startConfigPortal(deviceId);  
  }

  // When wifi is already connected but connection got interrupted ...
  if (((configCount > 1 && mqttClient.connected() && WiFi.status() == WL_CONNECTED) || (configCount > 1 && !mqttClient.connected() && WiFi.status() == WL_CONNECTED)) && manualConfig == false) { 
    Serial.println(F("User config access"));
    wifiManager.setConfigPortalTimeout(configTimeout2);
    wifiManager.startConfigPortal(config.mqtt_client, devicePass);
    long now = millis();
    if (now - lastMqttReconnectAttempt > 5000) {
      lastMqttReconnectAttempt = now;
      if (mqttConnect()) {
          #if DEBUG == 1
            Serial.print(F("Attempting MQTT connection to : "));
            Serial.println((const char*)config.mqtt_server);
          #endif
          lastMqttReconnectAttempt = 0;
      }
    }  
  }
  
  // After first start, hard reset, or without known WiFi AP
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Auto config access"));
    //wifiManager.setConfigPortalTimeout(configTimeout1);
    if (!wifiManager.autoConnect(config.mqtt_client, devicePass)) {
      Serial.println(F("Connection failure --> Timeout"));
      delay(3000);
      setReboot();
    }
  }

  else {
    
    if (shouldSaveConfig) {
      //config.mqtt_client = ;
      strlcpy(config.mqtt_server, custom_mqtt_server.getValue(), sizeof(config.mqtt_server));
      strlcpy(config.mqtt_port, custom_mqtt_port.getValue(),  sizeof(config.mqtt_port));         
      strlcpy(config.mqtt_user, custom_mqtt_user.getValue(),  sizeof(config.mqtt_user));         
      strlcpy(config.mqtt_password, custom_mqtt_password.getValue(),  sizeof(config.mqtt_password));
      strlcpy(config.mqtt_topic_in, config.mqtt_client,  sizeof(config.mqtt_topic_in));
      strcat(config.mqtt_topic_in,message.in_prefix); 
      strlcpy(config.mqtt_topic_out, config.mqtt_client,  sizeof(config.mqtt_topic_out));
      strcat(config.mqtt_topic_out,message.out_prefix); 

      //strcat(config.mqtt_topic_in,message.out_prefix); 
      Serial.println(F("Saving config"));
      
//      StaticJsonDocument<objBufferSize> doc; 
//      JsonObject& obj = doc.to<JsonObject>();
      StaticJsonBuffer<(objBufferSize * 2)> doc;
      JsonObject& obj = doc.createObject();

      obj["mqtt_server"] = config.mqtt_server;
      obj["mqtt_port"] = config.mqtt_port;
      obj["mqtt_client"] = config.mqtt_client;
      obj["mqtt_user"] = config.mqtt_user;
      obj["mqtt_password"] = config.mqtt_password;
      obj["mqtt_topic_in"] = config.mqtt_topic_in;
      obj["mqtt_topic_out"] = config.mqtt_topic_out;
      //obj.set<char>("mqtt_server", mqtt_server);
      File configFile = SPIFFS.open("/config.json", "w");
      if (!configFile) {
        Serial.println(F("Failed to open config file"));
      }
//      if (serializeJsonPretty(doc, Serial) == 0) {
//        Serial.println(F("Failed to write to Serial"));
//      } 
//      if (serializeJson(doc, configFile) == 0) {
//        Serial.println(F("Failed to write to file"));
//      } 
      obj.printTo(Serial);
      obj.printTo(configFile);
      configFile.close();
      Serial.println();
    }  
    Serial.println(F("Wifi config mode closed"));
    Serial.print(F("Config mode counter : ")); Serial.println(configCount);
    configMode = 0;
    ticker.detach();
    if (STATE_LED == LOW); digitalWrite(STATE_LED, HIGH);
    //delay(100);
    Serial.printf("config heap size: %u\n", ESP.getFreeHeap());
  //}
       
  Serial.println();
  Serial.println(F("====== Config successful ========="));
  Serial.print(F("IP address : ")); Serial.println(WiFi.localIP()); 
  Serial.println();

  mqttInit();

  }
}
