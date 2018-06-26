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
  if (mqttClient.connect((mqttClientId), (mqttUser), (mqttPassword))) {
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

  if ( topic == mqttTopicIn1 ) {
    if (s == "capture") {
      serverCapture();
    }
    if (s == "stream") {
      serverStream();
    }
  }
  
  if ( topic == mqttTopicIn2 ) {
    int reso = s.toInt();
    Serial.print("reso");
    Serial.println(reso);
    if ( reso >= 0 || reso <= 8 ) {
      setCamResolution(reso);
    }
  }

  if ( topic == mqttTopicIn3 ) {
    int fpm = s.toInt();
    Serial.print("fpm");
    Serial.println(fpm);
    if ( fpm >= 0 || fpm <= 4 ) {
      setFPM(fpm);
    }
  }
  
  if ( topic == mqttTopicIn4 ) { 
    if (s == "update") {
        //getUpdated();
    }
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
        //strtok(s, "-");
        // extract otaSignal, otaType, otaUrl, and fingerprint ( for httpsUpdate )
        // if the int correspond to waited value, update otaSignal, otaFile and setReboot()
      }
  }
  
  else {
    mqttError(1);
    Serial.print(F("Message arrived ["));
    Serial.print(topic);
    Serial.print(F("] "));
    Serial.println(s);
    Serial.println();
  }
 
}

void mqttError(int e) {
  if ( e = 1 ) {
    #if DEBUG == 1
      Serial.print(F("Message not handled ["));

    #endif
  }
  if ( e = 2 ) {
     ticker.attach(0.3, tick);
     #if DEBUG == 1
      Serial.print(F("Attempting MQTT connection to : "));
      Serial.println(mqttServer);
     #endif
  }
  if ( e = 3 ) {
 //   mqttReconnect() = false;
    ticker.attach(0.3, tick);
    Serial.println(F("Connexion MQTT infructueuse après 5 essais --> mode config"));
    lastMqttReconnectAttempt = 0;
    mqttFailCount = 0;
    configManager();
  }
}

