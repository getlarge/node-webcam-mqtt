///////////////////////////////////////////////////////////////////////////////////
//   handle mqtt init, connect, reconnect, callback      //
///////////////////////////////////////////////////////////////////////////////////
void mqttInit() {
  mqttClient.setServer(config.mqtt_server, atoi(config.mqtt_port));
  mqttClient.setCallback(mqttCallback);
  mqttClient.connect(((const char*)config.mqtt_client), ((const char*)config.mqtt_user), ((const char*)config.mqtt_password));
  //mqttClient.publish((const char*)strcat(config.mqtt_topic_out,"/logs" ), "Check 1-2 1-2");
  mqttClient.subscribe((const char*)strcat(config.mqtt_topic_in,"/+/+" ));
  Serial.printf("Connecting to MQTT broker %s:%i as %s\n", (const char*)config.mqtt_server, atoi(config.mqtt_port), (const char*)config.mqtt_client);
  
}

boolean mqttConnect() {
  if (mqttClient.connect(((const char*)config.mqtt_client), ((const char*)config.mqtt_user), ((const char*)config.mqtt_password))) {
    Serial.println(F("MQTT connected"));
    mqttClient.subscribe((const char*)strcat(config.mqtt_topic_in,"/+/+" ));
  }
  return mqttClient.connected();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char *str, *p;
  uint8_t i = 0;
  
// first sanity check
//  if (topic != strstr(topic, config.mqtt_topic_in)) {
//    Serial.print("faux protocole!");
//    return;
//  }

// parse the MQTT protocol ( +deviceId/+inPrefix/+sensor/+command )
  for (str = strtok_r(topic + 1, "/", &p); str && i <= 3;
       str = strtok_r(NULL, "/", &p)) {
    switch (i) {
      case 0: {
        //deviceId
        break;
      }
      case 1: {
        //inPrefix = str;
        break;
      }
      case 2: {
        message.sensor = str;
        break;
      }
      case 3: {
        message.command = str;
        break;
      }
    }
    i++;
  }
  payload[length] = '\0';
  message.payload = ((char*)payload);
  Serial.printf("Received command : %s \n", (const char*)message.command);
  Serial.printf("For sensor : %s \n", (const char*)message.sensor);
  Serial.printf("Payload : %s \n", (const char*)message.payload);
  /// Export the following in a parsing function later, like parseMessage(message)....
  if ( message.sensor = "camera" ) {
    if ( message.command = "capture" ) {
      if ( message.payload = "1" ) {
         Serial.println("ca-me-ra-me-raaaaaaaa");
        // get the payload to add parameters in the function ?
        return serverCapture();
      }
    }
    if ( message.command = "timelapse" ) {
      if ( message.payload = "1" ) {
        // get from the payload to add timing options ?
        timelapse = true;
        return;
      }
    }
    if ( message.command = "stream" ) {
      if ( message.payload = "1" ) {
        transmitStream = true;
        return serverStream();
      }
      if ( message.payload = "0" ) {
       transmitStream = false;
       return;
      }
    }
  }
  if ( message.sensor = "system" ) {
    if ( message.command = "reso" ) {
      //int reso = message.payload.toInt();
      int reso = atoi(message.payload);
      Serial.print("reso");
      Serial.println(reso);
      if ( reso >= 0 || reso <= 8 ) {
        setCamResolution(reso);
        return updateFile(resFile, reso);
      }
    }
    if ( message.command = "fpm" ) {
      int fpm = atoi(message.payload); //.toInt();
      Serial.print("fpm");
      Serial.println(fpm);
      if ( fpm >= 0 || fpm <= 4 ) {
        setFPM(fpm);
        return updateFile(fpmFile, fpm);
      }
    }
    if ( message.command = "update" ) {
      if (message.payload = "ota") {
        //return getUpdated();
      }
      for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
        //strtok(s, "-");
        // extract otaSignal, otaType, otaUrl, and fingerprint ( for httpsUpdate )
        // if the int correspond to waited value, update otaSignal, otaFile and setReboot()
      }
    }
  }

  else {
    //mqttError(1);
    //return;
  }

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

