///////////////////////////////////////////////////////////////////////////////////
//    function to modifiy, save, settings for connection to wifi and server      //
///////////////////////////////////////////////////////////////////////////////////
void configManager() {
  
WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
//WiFiManagerParameter custom_mqtt_client_id("client", "mqtt client", mqtt_client_id, 40);
WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 20);
WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, 30);
WiFiManagerParameter custom_http_server("httpServer", "http server", http_server, 40);
WiFiManagerParameter custom_http_port("httpPort", "http port", http_port, 6);

  Serial.println(F("====== Wifi config mode opening ========="));
  ticker.attach(0.5, tick);
  configCount++;
  
  WiFiManager wifiManager;
#ifdef MY_DEBUG 
  wifiManager.setDebugOutput(true);
#endif
#ifndef MY_DEBUG 
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
  //wifiManager.addParameter(&custom_mqtt_client_id);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_http_server);
  wifiManager.addParameter(&custom_http_port);

  configMode = 1;
  Serial.print("mqttServer:");
  Serial.println(mqttServer);

 // When no credentials or asking ...
  if (( !mqttServer ) || (configCount > 1 && manualConfig == true)) {
    Serial.println(F("Manual config access"));
    wifiManager.setConfigPortalTimeout(configTimeout1);
    //wifiManager.startConfigPortal(deviceId, devicePass);
    wifiManager.startConfigPortal(deviceId);  
  }

  // When wifi is already connected but connection got interrupted ...
  if (((configCount > 1 && mqttClient.connected() && WiFi.status() == WL_CONNECTED) || (configCount > 1 && !mqttClient.connected() && WiFi.status() == WL_CONNECTED)) && manualConfig == false) { 
    Serial.println(F("User config access"));
    wifiManager.setConfigPortalTimeout(configTimeout2);
    wifiManager.startConfigPortal(deviceId);
    long now = millis();
    if (now - lastMqttReconnectAttempt > 5000) {
      lastMqttReconnectAttempt = now;
      if (mqttConnect()) {
          #if DEBUG == 1
            Serial.print(F("Attempting MQTT connection to : "));
            Serial.println(mqttServer);
          #endif
          lastMqttReconnectAttempt = 0;
      }
    }  
  }
  
  // After first start, hard reset, or without known WiFi AP
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Auto config access"));
    //wifiManager.setConfigPortalTimeout(configTimeout1);
    if (!wifiManager.autoConnect(deviceId)) {
      Serial.println(F("Connection failure --> Timeout"));
      delay(3000);
     //reset and try again, or maybe put it to deep sleep
      setReboot();
    }
  }

  else {
    Serial.print(F("Config mode counter : "));
    Serial.println(configCount);
    
    if (shouldSaveConfig) {
      strcpy(mqtt_server, custom_mqtt_server.getValue()); 
      strcpy(mqtt_port, custom_mqtt_port.getValue());
      strcpy(mqtt_client_id, deviceId); 
      //strcpy(mqtt_client_id, custom_mqtt_client_id.getValue()); 
      strcpy(mqtt_user, custom_mqtt_user.getValue());
      strcpy(mqtt_password, custom_mqtt_password.getValue());
      strcpy(http_server, custom_http_server.getValue()); 
      strcpy(http_port, custom_http_port.getValue());
      Serial.println(F("Saving config"));
      DynamicJsonDocument doc;
      JsonObject& json = doc.to<JsonObject>();
      json["mqtt_server"] = mqtt_server;
      json["mqtt_port"] = mqtt_port;
      json["mqtt_client_id"] = mqtt_client_id;
      json["mqtt_user"] = mqtt_user;
      json["mqtt_password"] = mqtt_password;
      json["http_server"] = http_server;
      json["http_port"] = http_port;
      strcpy(mqtt_topic_in,mqtt_client_id);
      strcat(mqtt_topic_in,in);
      strcpy(mqtt_topic_out,mqtt_client_id);
      strcat(mqtt_topic_out,out);
      strcpy(mqtt_topic_out2,mqtt_client_id);
      strcat(mqtt_topic_out2,out2);
      strcpy(post_destination,post_prefix);
      strcat(post_destination,mqtt_client_id); 
      mqttServer = mqtt_server;
      mqttPort = atoi(mqtt_port);
      mqttClientId = mqtt_client_id;
      mqttUser = mqtt_user;
      mqttPassword = mqtt_password;
      mqttTopicIn = mqtt_topic_in;
      mqttTopicOut = mqtt_topic_out;
      mqttTopicOut2 = mqtt_topic_out2;
      httpServer = http_server;
      httpPort = atoi(http_port);
      postDestination = post_destination;
      File configFile = SPIFFS.open("/config.json", "w");
      if (!configFile) {
        Serial.println(F("Failed to open config file"));
      }
      serializeJson(doc, Serial);
      serializeJson(doc, configFile);
      configFile.close();
    }
  //  Serial.println(F("Connected"));
  
    Serial.println(F("Wifi config mode closed"));
    configMode = 0;
    ticker.detach();
    digitalWrite(STATE_LED, HIGH);
    delay(100);
    Serial.printf("config heap size: %u\n", ESP.getFreeHeap());
  }
       
  Serial.println();
  Serial.println(F("====== Connections successful ========="));
  Serial.print(F("IP address : "));
  Serial.println(WiFi.localIP()); 
  Serial.println(F("Config User : "));
  Serial.print(mqttClientId);
  Serial.print(F(" | "));
  Serial.println(mqttUser);
  Serial.println(F("Config MQTT : "));
  Serial.print(mqttServer);
  Serial.print(F(":"));
  Serial.println(mqttPort);
  Serial.print(mqttTopicIn);
  Serial.print(F(" | "));
  Serial.println(mqttTopicOut);
  Serial.println(F("Config HTTP: "));
  Serial.print(httpServer);
  Serial.print(postDestination);
  Serial.print(F(":"));
  Serial.println(httpPort);
  Serial.println(F("=============================="));
  Serial.println();
  }
