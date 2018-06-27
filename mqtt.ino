///////////////////////////////////////////////////////////////////////////////////
//   handle mqtt init, connect, reconnect, callback      //
///////////////////////////////////////////////////////////////////////////////////
void mqttInit() {
  mqttClient.setServer(config.mqtt_server, atoi(config.mqtt_port));
  mqttClient.setCallback(mqttCallback);
  mqttClient.connect(((const char*)deviceId), ((const char*)config.mqtt_user), ((const char*)config.mqtt_password));
  mqttClient.publish((const char*)config.mqtt_topic_out, "Check 1-2 1-2");
  mqttClient.subscribe((const char*)config.mqtt_topic_in);
  Serial.printf("Connecting to MQTT broker %s:%i as %s\n", (const char*)config.mqtt_server, atoi(config.mqtt_port), (const char*)deviceId);
}

boolean mqttConnect() {
  if (mqttClient.connect(((const char*)deviceId), ((const char*)config.mqtt_user), ((const char*)config.mqtt_password))) {
#if DEBUG == 1
    Serial.println(F("MQTT connected"));
#endif
    mqttClient.publish((const char*)config.mqtt_topic_out, "Check 1-2 1-2");
    mqttClient.subscribe((const char*)config.mqtt_topic_in);
  }
  return mqttClient.connected();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {

  char *str, *p;
  uint8_t i = 0;
  const char *sensor;
  const char *comm;
  
// check it protocol is right 
//  if (topic != strstr(topic, mqttTopicIn)) {
//    return false;
//    Serial.print("faux!");
//  }
//  Serial.printf("Message arrived [%s] %s \n", (const char*)topic, (const char*)(char*)payload);

// parse the MQTT protocol
  for (str = strtok_r(topic + 1, "/", &p); str && i <= 2;
       str = strtok_r(NULL, "/", &p)) {
    switch (i) {
      case 0: {
          //device id
          break;
        }
      case 1: {
          // "in"
          break;
        }
      case 2: {
          comm = str;
          //mSetCommand(message, command);
          break;
        }
    }
    i++;
  }
  Serial.print(comm);
  
  payload[length] = '\0';
  String s = String((char*)payload);
  
  if ( comm == "capture" ) {
    if (s == "capture") {
      serverCapture();
    }
    if (s == "stream") {
      serverStream();
    }
  }

  if ( comm == "reso" ) {
    int reso = s.toInt();
    Serial.print("reso");
    Serial.println(reso);
    if ( reso >= 0 || reso <= 8 ) {
      setCamResolution(reso);
      updateFile(resFile, reso);
    }
  }

  if ( comm == "fpm" ) {
    int fpm = s.toInt();
    Serial.print("fpm");
    Serial.println(fpm);
    if ( fpm >= 0 || fpm <= 4 ) {
      setFPM(fpm);
      updateFile(fpmFile, fpm);
    }
  }

  if ( comm == "system" ) {
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

//  else {
//    mqttError(1);
//    
//  }

}

void mqttError(unsigned int e) {
  static int context = e;
  if ( context = 1 ) {
#if DEBUG == 1
    Serial.println(F("Message not handled"));
#endif
  }
  if (context = 2 ) {
    checkButton(0);
    ++mqttFailCount;
    ticker.attach(0.3, tick);
#if DEBUG == 1
    Serial.print(F("Attempting MQTT connection to : "));
    Serial.println((const char*)config.mqtt_port);
    Serial.println(mqttFailCount);
    return;
#endif
  }
  if ( context = 3 ) {
    //   mqttReconnect() = false;
    
  }
}

