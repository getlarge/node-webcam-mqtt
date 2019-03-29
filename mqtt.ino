///////////////////////////////////////////////////////////////////////////////////
//   handle mqtt init, connect, reconnect, callback, message parsing      //
///////////////////////////////////////////////////////////////////////////////////
void mqttInit(Config &config) {
  aSerial.vvv().p(F("Init mqtt://")).p((const char*)config.mqttServer).p(":").pln(atoi(config.mqttPort));
  mqttClient.setServer(config.mqttServer, atoi(config.mqttPort));
  mqttClient.setCallback(mqttCallback);
}

void generateMqttClientId(Config &config) {
  strcpy(config.mqttClient, config.devEui);
  long randNumber = random(10000);
  char randNumberBuffer[10];
  ltoa(randNumber, randNumberBuffer, 10);
  strcat(config.mqttClient, "-" );
  strcat(config.mqttClient, randNumberBuffer);
}

boolean mqttConnect(Config &config) {
  aSerial.vvv().p(F("Connecting to mqtt://")).p((const char*)config.mqttServer).p(":").p(atoi(config.mqttPort)).p(F(" - client ID : ")).pln((const char*)config.mqttClient);
  if (mqttClient.connect(((const char*)config.mqttClient), ((const char*)config.mqttUser), ((const char*)config.mqttPassword))) {
    mqttClient.subscribe((const char*)message.masterTopic);
    presentSensors("3306", "1", "5850", "0");
    presentSensors("3349", "2", "5910", "0");
  }
  return mqttClient.connected();
}

void mqttReconnect(Config &config) {
  // Loop until we're reconnected
  mqttFailCount = 0;
//  if ( (strcmp((const char*)config.mqttServer, "") == 0) ) {
//    return configManager(config);
//  }
//  if ( (strcmp((const char*)config.mqttPort, "") == 0) ) {
//    return configManager(config);
//  }
  while (!mqttClient.connected()) {
    ++mqttFailCount;
    aSerial.vvv().p(F("Connecting to mqtt://")).p((const char*)config.mqttServer).p(":").pln(atoi(config.mqttPort));
    generateMqttClientId(config);
    if (mqttClient.connect(((const char*)config.mqttClient), ((const char*)config.mqttUser), ((const char*)config.mqttPassword))) {
      aSerial.vvv().p(F("Connected to mqtt as : ")).pln((const char*)config.mqttClient);
      mqttClient.subscribe((const char*)message.masterTopic);
      presentSensors("3306", "1", "5850", "0");
      presentSensors("3349", "2", "5910", "0");
    } else {
      aSerial.vvv().p(F("Failed mqtt connection : ")).pln(mqttClient.state());
      if (mqttFailCount > mqttMaxFailedCount) {
        aSerial.vv().p(mqttMaxFailedCount).pln(F("+ MQTT connection failure --> config mode"));
        return configManager(config);
      }
      delay(reconnectInterval);
    }
  }
}

void presentSensors(const char* objectId, const char* sensorId, const char* resourceId, const char* payload) {
  // "pattern": "+prefixedDevEui/+method/+omaObjectId/+sensorId/+ipsoResourceId",
  char presentationTopic[70];
  strlcpy(presentationTopic, config.mqttTopicOut, sizeof(presentationTopic));
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
  //  if (topic != strstr(topic, config.mqttTopicIn)) {
  //    Serial.print("error in the protocol");
  //    return;
  //  }

  // "pattern": "+prefixedDevEui/+method/+omaObjectId/+sensorId/+omaResourcesId",
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
          strlcpy(message.omaObjectId, str, sizeof(message.omaObjectId));
          break;
        }
      case 3: {
          strlcpy(message.sensorId, str, sizeof(message.sensorId));
          break;
        }
      case 4: {
          strlcpy(message.omaResourcesId, str, sizeof(message.omaResourcesId));
          break;
        }
    }
    i++;
  }
  aSerial.vvv().p(F("Method : ")).pln((const char*)message.method).p(F("For sensor : ")).pln((const char*)message.omaObjectId);
  parseMessage(message);
}

void parseMessage(Message &message) {
  if ( strcmp(message.omaObjectId, "3349") == 0 && strcmp(message.omaResourcesId, "5911") == 0) {
    if ( strcmp(message.method, "1") == 0 ) {
      if ( strcmp(message.payload, "true") == 0 ) {
        aSerial.vvvv().pln(F("ca-me-ra-me-raaaaaaaa"));
        return serverCapture(myCAM, message);
      }
    }
    if ( strcmp(message.method, "4") == 0 ) {
      if ( strcmp(message.payload, "1") == 0 ) {
        transmitStream = true;
        return serverStream(myCAM, message);
      }
      if ( strcmp(message.payload, "0") == 0) {
        transmitStream = false;
        return;
      }
    }
  }
  else if ( ( strcmp(message.method, "1") == 0 ) && ( strcmp(message.omaObjectId, "2000") == 0 ) ) {
    if ( strcmp(message.omaResourcesId, "reso") == 0 ) {
      //  int reso = atoi(message.payload);
      const char* reso = message.payload;
      aSerial.vv().p(F("Change resolution to : ")).pln(reso);
      if ( atoi(reso) >= 0 || atoi(reso) <= 8 ) {
        strlcpy(config.camResolution, reso,  sizeof(config.camResolution));
        setCamResolution(atoi(reso));
        //  return saveConfig(configFileName, config);
      }
    }
    if ( strcmp(message.omaResourcesId, "fpm") == 0 ) {
      //  int fpm = atoi(message.payload);
      const char* fpm = message.payload;
      aSerial.vv().p(F("Change FPM to : ")).pln(fpm);
      if ( atoi(fpm) >= 0 || atoi(fpm) <= 4 ) {
        strlcpy(config.camFpm, fpm,  sizeof(config.camFpm));
        setFPM(atoi(fpm));
        //  return saveConfig(configFileName, config);
      }
    }
    if ( strcmp(message.omaResourcesId, "update") == 0 ) {
      if (strcmp(message.payload, "ota") == 0) {
        //  strtok(s, "-");
        //  extract otaSignal, otaType, otaUrl, and fingerprint ( for httpsUpdate )
        //  if the int correspond to waited value, update otaSignal, otaFile and setReboot()
        //  return getUpdated();
      }

    }
  }
  else {
    //mqttError(1);
    //return;
  }
}
