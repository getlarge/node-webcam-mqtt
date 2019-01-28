///////////////////////////////////////////////////////////////////////////////////
//   handle mqtt init, connect, reconnect, callback, message parsing      //
///////////////////////////////////////////////////////////////////////////////////
void mqttInit() {
  mqttClient.setServer(config.mqtt_server, atoi(config.mqtt_port));
  mqttClient.setCallback(mqttCallback);
  mqttClient.connect(((const char*)config.mqtt_client), ((const char*)config.mqtt_user), ((const char*)config.mqtt_password));
  mqttClient.subscribe((const char*)masterTopic);
  presentSensors("3349", "0", "5910", "0");
  presentSensors("3306", "1", "5850", "0");
  aSerial.vv().p(F("Connecting to mqtt://")).p((const char*)config.mqtt_server).p(":").p(atoi(config.mqtt_port)).p(F(" - client ID : ")).pln((const char*)config.mqtt_client);
}

boolean mqttConnect() {
  if (mqttClient.connect(((const char*)config.mqtt_client), ((const char*)config.mqtt_user), ((const char*)config.mqtt_password))) {
    aSerial.vv().pln(F("MQTT client connected"));
    mqttClient.subscribe((const char*)masterTopic);
    presentSensors("3306", "1", "5850", "0");
    presentSensors("3349", "2", "5910", "0");
  }
  return mqttClient.connected();
}

void presentSensors(const char* objectId, const char* sensorId, const char* resourceId, const char* payload) {
  // "pattern": "+prefixedDevEui/+method/+ipsoObjectId/+sensorId/+ipsoResourceId",
  char presentationTopic[70];
  strlcpy(presentationTopic, config.mqtt_topic_out, sizeof(presentationTopic));
  strcat(presentationTopic, "/0/" );
  strcat(presentationTopic, objectId );
  strcat(presentationTopic, "/" );
  strcat(presentationTopic, sensorId );
  strcat(presentationTopic, "/" );
  strcat(presentationTopic, resourceId );
  aSerial.vvv().p(F("presentation :")).pln((const char*)presentationTopic);
  mqttClient.publish((const char*)presentationTopic, payload);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char *str, *p;
  uint8_t i = 0;
  payload[length] = '\0';
  message.payload = ((char*)payload);
  aSerial.v().println(F("====== Received message ======"));
  aSerial.vv().p(F("Topic : ")).pln((const char*)topic).p(F("Payload : ")).pln((const char*)message.payload);
  // first sanity check
  //  if (topic != strstr(topic, config.mqtt_topic_in)) {
  //    Serial.print("error in the protocol");
  //    return;
  //  }
  // "pattern": "+prefixedDevEui/+method/+ipsoObjectId/+sensorId/+ipsoResourcesId",
  for (str = strtok_r(topic + 1, "/", &p); str && i <= 4;
       str = strtok_r(NULL, "/", &p)) {
    switch (i) {
      case 0: {
          //prefixedDevEui
          break;
        }
      case 1: {
          strlcpy(message.method, str, sizeof(message.method));
          break;
        }
      case 2: {
          strlcpy(message.ipsoObjectId, str, sizeof(message.ipsoObjectId));
          break;
        }
      case 3: {
          strlcpy(message.sensorId, str, sizeof(message.sensorId));
          break;
        }
      case 4: {
          strlcpy(message.ipsoResourcesId, str, sizeof(message.ipsoResourcesId));
          break;
        }
    }
    i++;
  }
  aSerial.vvv().p(F("Received command : ")).pln((const char*)message.method).p(F("For sensor : ")).pln((const char*)message.ipsoObjectId);
  parseMessage(message);

  //  for (int i = 0; i < length; i++) {
  //    aSerial.vv().p((char)payload[i]);
  //    //strtok(s, "-");
  //    // extract otaSignal, otaType, otaUrl, and fingerprint ( for httpsUpdate )
  //    // if the int correspond to waited value, update otaSignal, otaFile and setReboot()
  //  }

}

void parseMessage(messageFormat message) {
  if ( strcmp(message.ipsoObjectId, "3349") == 0 && strcmp(message.ipsoResourcesId, "5911") == 0) {
    if ( strcmp(message.method, "1") == 0 ) {
      if ( strcmp(message.payload, "1") == 0 ) {
        aSerial.vvvv().pln(F("ca-me-ra-me-raaaaaaaa"));
        return serverCapture();
      }
    }
    //    if ( strcmp(message.method, "timelapse") == 0 ) {
    //      if ( strcmp(message.payload, "1") == 0 ) {
    //        timelapse = true;
    //        return;
    //      }
    //      if ( strcmp(message.payload, "0") ==  0) {
    //        timelapse = false;
    //        return;
    //      }
    //    }
    if ( strcmp(message.method, "4") == 0 ) {
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
  else if ( ( strcmp(message.method, "1") == 0 ) && ( strcmp(message.ipsoObjectId, "2000") == 0 ) ) {
    if ( strcmp(message.ipsoResourcesId, "reso") == 0 ) {
      int reso = atoi(message.payload);
      aSerial.vv().p(F("Change resolution to : ")).pln(reso);
      if ( reso >= 0 || reso <= 8 ) {
        setCamResolution(reso);
        return updateFile(resFile, reso);
      }
    }
    if ( strcmp(message.ipsoResourcesId, "fpm") == 0 ) {
      int fpm = atoi(message.payload);
      aSerial.vv().p(F("Change FPM to : ")).pln(fpm);
      if ( fpm >= 0 || fpm <= 4 ) {
        setFPM(fpm);
        return updateFile(fpmFile, fpm);
      }
    }
    if ( strcmp(message.ipsoResourcesId, "update") == 0 ) {
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
