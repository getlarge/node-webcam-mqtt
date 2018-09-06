///////////////////////////////////////////////////////////////////////////////////
//   handle mqtt init, connect, reconnect, callback, message parsing      //
///////////////////////////////////////////////////////////////////////////////////
void mqttInit() {
  mqttClient.setServer(config.mqtt_server, atoi(config.mqtt_port));
  mqttClient.setCallback(mqttCallback);
  mqttClient.connect(((const char*)config.mqtt_client), ((const char*)config.mqtt_user), ((const char*)config.mqtt_password));
  mqttClient.subscribe((const char*)masterTopic);
  aSerial.vv().p(F("Connecting to mqtt://")).p((const char*)config.mqtt_server).p(":").p(atoi(config.mqtt_port)).p(F(" - client ID : ")).pln((const char*)config.mqtt_client);
}

boolean mqttConnect() {
  if (mqttClient.connect(((const char*)config.mqtt_client), ((const char*)config.mqtt_user), ((const char*)config.mqtt_password))) {
    aSerial.vv().pln(F("MQTT client connected"));
    mqttClient.subscribe((const char*)masterTopic);
  }
  return mqttClient.connected();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char *str, *p;
  uint8_t i = 0;
  payload[length] = '\0';
  message.payload = ((char*)payload);
  aSerial.vv().p(F("Topic : ")).pln((const char*)topic).p(F("Payload : ")).pln((const char*)message.payload);
  // first sanity check
  //  if (topic != strstr(topic, config.mqtt_topic_in)) {
  //    Serial.print("error in the protocol");
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
          strlcpy(message.sensor, str, sizeof(message.sensor));
          break;
        }
      case 3: {
          strlcpy(message.command, str, sizeof(message.command));
          break;
        }
    }
    i++;
  }
  aSerial.vvv().p(F("Received command : ")).pln((const char*)message.command).p(F("For sensor : ")).pln((const char*)message.sensor);

  parseMessage(message);
//  for (int i = 0; i < length; i++) {
//    aSerial.vv().p((char)payload[i]);
//    //strtok(s, "-");
//    // extract otaSignal, otaType, otaUrl, and fingerprint ( for httpsUpdate )
//    // if the int correspond to waited value, update otaSignal, otaFile and setReboot()
//  }

}

void parseMessage(messageFormat message) {
  if ( strcmp(message.sensor, "camera") == 0) {
    if ( strcmp(message.command, "capture") == 0 ) {
      if ( strcmp(message.payload, "1") == 0 ) {
        aSerial.vvvv().pln(F("ca-me-ra-me-raaaaaaaa"));
        return serverCapture();
      }
    }
    if ( strcmp(message.command, "timelapse") == 0 ) {
      if ( strcmp(message.payload, "1") == 0 ) {
        timelapse = true;
        return;
      }
      if ( strcmp(message.payload, "0") ==  0) {
        timelapse = false;
        return;
      }
    }
    if ( strcmp(message.command, "stream") == 0 ) {
      if ( strcmp(message.payload, "1") == 0 ) {
        transmitStream = true;
        return serverStream();
      }
      if ( strcmp(message.payload, "0") == 0) {
        transmitStream = false;
        return;
      }
    }
  }
  if ( strcmp(message.sensor, "system") == 0 ) {
    if ( strcmp(message.command, "reso") == 0 ) {
      int reso = atoi(message.payload);
      aSerial.vv().p(F("Change resolution to : ")).pln(reso);
      if ( reso >= 0 || reso <= 8 ) {
        setCamResolution(reso);
        return updateFile(resFile, reso);
      }
    }
    if ( strcmp(message.command, "fpm") == 0 ) {
      int fpm = atoi(message.payload);
      aSerial.vv().p(F("Change FPM to : ")).pln(fpm);
      if ( fpm >= 0 || fpm <= 4 ) {
        setFPM(fpm);
        return updateFile(fpmFile, fpm);
      }
    }
    if ( strcmp(message.command, "update") == 0 ) {
      if (strcmp(message.payload, "ota") == 0) {
        //return getUpdated();
      }

    }
  }
  else {
    //mqttError(1);
    //return;
  }
}

