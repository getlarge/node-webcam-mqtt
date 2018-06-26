///////////////////////////////////////////////////////////////////////////////////
//   handle mqtt init, connect, reconnect, callback      //
///////////////////////////////////////////////////////////////////////////////////
void mqttInit() {
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.connect((mqttClientId),(mqttUser),(mqttPassword));
  mqttClient.publish(mqttTopicOut, "Check 1-2 1-2");
  mqttClient.subscribe(mqttTopicIn);
  Serial.println(F("Connected to MQTT server: "));
  Serial.println(mqttServer);
}

boolean mqttConnect() {
//  String clientId = "Arducam-";
//  clientId += String(random(0xffff), HEX);  Serial.println(F("Attempting MQTT connection..."));
  if (mqttClient.connect((mqttClientId), (mqttUser), (mqttPassword))) {
  //if (MqttClient.connect(clientId.c_str(), (mqttUser), (mqttPassword))) {
    #if DEBUG == 1
      Serial.println(F("MQTT connected"));
    #endif 
    mqttClient.publish(mqttTopicOut, "Check 1-2 1-2");
    mqttClient.subscribe(mqttTopicIn);
  }
  mqttFailCount = 0;
  return mqttClient.connected();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String s = String((char*)payload);
  Serial.print(F("Message arrived ["));
  Serial.print(topic);
  Serial.print(F("] "));
  Serial.println(s);
  //    for (int i = 0; i < length; i++) {
  //    Serial.print((char)payload[i]);
  //    }
  Serial.println();
  if (s == "capture") {
    serverCapture();
  }
  if (s == "off") {
    switchOnCam = 0;
  }
  if (s == "on") {
    switchOnCam = 1;
  }
  if (s == "update") {
    if ((WiFi.status() == WL_CONNECTED)) {
      // MqttClient.disconnect();
      // MqttWifiClient.stop();
      // delay(1000);
      ESPhttpUpdate.rebootOnUpdate(true);
      Serial.println(F("Update SPIFFS..."));
      //t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs(otaUrl, "", httpsFingerprint);
      t_httpUpdate_return ret = ESPhttpUpdate.update(otaUrl);
      //  t_httpUpdate_return ret = ESPhttpUpdate.update(otaUrl, "", httpsFingerprint);

      // if(ret == HTTP_UPDATE_OK) {
      //   Serial.println(F("Update sketch..."));
      //   ret = ESPhttpUpdate.update(otaUrl, "", httpsFingerprint);
      // ret = ESPhttpUpdate.update(host, httpsPort, url, currentVersion, httpsFingerprint);
      switch (ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          Serial.println();
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Serial.println(F("HTTP_UPDATE_NO_UPDATES"));
          break;

        case HTTP_UPDATE_OK:
          Serial.println(F("HTTP_UPDATE_OK"));
          break;
      }
    }
  }
  if (s == "reso0") {
    setCamResolution(0);
    updateResolutionFile();
  }
  if (s == "reso1") {
    setCamResolution(1);
    updateResolutionFile();
  }
  if (s == "reso2") {
    setCamResolution(2);
    updateResolutionFile();
  }
  if (s == "reso3") {
    setCamResolution(3);
    updateResolutionFile();
  }
  if (s == "reso4") {
    setCamResolution(4);
    updateResolutionFile();
  }
  if (s == "reso5") {
    setCamResolution(5);
    updateResolutionFile();
  }
  if (s == "reso6") {
    setCamResolution(6);
    updateResolutionFile();
  }
  if (s == "reso7") {
    setCamResolution(7);
    updateResolutionFile();
  }
  if (s == "reso8") {
    setCamResolution(8);
    updateResolutionFile();
  }
  if (s == "int0") {
    setFPM(0);
    updateFPMFile();
  }
  if (s == "fpm0") {
    setFPM(0);
    updateFPMFile();
  }
  if (s == "fpm1") {
    setFPM(1);
    updateFPMFile();
  }
  if (s == "fpm2") {
    setFPM(2);
    updateFPMFile();
  }
  if (s == "fpm3") {
    setFPM(3);
    updateFPMFile();
  }
  if (s == "fpm4") {
    setFPM(4);
    updateFPMFile();
  }
}

