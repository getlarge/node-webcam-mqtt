void setSensorRoutes(Config &config, const char* objectId, const char* sensorId, const char* resourceId, size_t index) {
  // "pattern": "+prefixedDevEui/+method/+omaObjectId/+sensorId/+ipsoResourceId",
  char topic[70];
  strcpy(topic, config.mqttTopicOut);
  strcat(topic, "/1/" );
  strcat(topic, objectId );
  strcat(topic, "/" );
  strcat(topic, sensorId );
  strcat(topic, "/" );
  strcat(topic, resourceId );
  strcpy(( char*)postTopics[index], topic);
  aSerial.vvvv().p(F("set route")).pln(( char*)postTopics[index]);
}

void setSensors(Config &config) {
  for (size_t i = 0; i < sizeof(sensors) / sizeof(sensors[0]); i++) {
    const char** sensor = sensors[i];
    char objectId[5];
    char sensorId[4];
    char resourceId[5];
    //  printf("sensor[%u]\n", i);
    for (size_t j = 0; sensor[j]; j++) {
      if (j == 0 ) {
        strlcpy(objectId, sensor[j], sizeof(objectId));
      } else if (j == 1 ) {
        strlcpy(sensorId, sensor[j], sizeof(sensorId));
      }  else if (j == 2 ) {
        strlcpy(resourceId, sensor[j], sizeof(resourceId));
      }
      //  printf("  [%s]\n", sensor[j]);
    }
    setSensorRoutes(config, (const char*)objectId, (const char*)sensorId, (const char*)resourceId, i);
  }
}

void presentSensors(Config &config) {
  for (size_t i = 0; i < sizeof(sensors) / sizeof(sensors[0]); i++) {
    const char** sensor = sensors[i];
    char objectId[5];
    char sensorId[4];
    char resourceId[5];
    char payload[30];
    //  printf("sensor[%u]\n", i);
    for (size_t j = 0; sensor[j]; j++) {
      if (j == 0 ) {
        strlcpy(objectId, sensor[j], sizeof(objectId));
      } else if (j == 1 ) {
        strlcpy(sensorId, sensor[j], sizeof(sensorId));
      }  else if (j == 2 ) {
        strlcpy(resourceId, sensor[j], sizeof(resourceId));
      } else if (j == 3 ) {
        strlcpy(payload, sensor[j], sizeof(payload));
      }
      //  printf("  [%s]\n", sensor[j]);
    }
    char presentationTopic[70];
    strlcpy(presentationTopic, config.mqttTopicOut, sizeof(presentationTopic));
    strcat(presentationTopic, "/0/" );
    strcat(presentationTopic, objectId );
    strcat(presentationTopic, "/" );
    strcat(presentationTopic, sensorId );
    strcat(presentationTopic, "/" );
    strcat(presentationTopic, resourceId );
    mqttClient.publish((const char*)presentationTopic, payload);
  }
}

void setMessage(Message &message, char method[5], char* objectId, char* sensorId, char* resourceId, char* payload ) {
  strlcpy(message.method, method, sizeof(method));
  strlcpy(message.omaObjectId, objectId, sizeof(objectId));
  strlcpy(message.sensorId, sensorId, sizeof(sensorId));
  strlcpy(message.omaResourceId, resourceId, sizeof(resourceId));
  strlcpy(message.payload, payload, sizeof(payload));
}

void sendMessage(Config &config, Message &message ) {
  char topic[70];
  strlcpy(topic, config.mqttTopicOut, sizeof(config.mqttTopicOut));
  strcat(topic, "/" );
  strcat(topic, message.method );
  strcat(topic, "/" );
  strcat(topic, message.omaObjectId );
  strcat(topic, "/" );
  strcat(topic, message.sensorId );
  strcat(topic, "/" );
  strcat(topic, message.omaResourceId );
  mqttClient.publish((const char*)topic, message.payload);
}

void parseMessage(Message &message) {
  for (size_t i = 0; i < sizeof(sensors) / sizeof(sensors[0]); i++) {
    const char** sensor = sensors[i];
    char objectId[5];
    char sensorId[4];
    char resourceId[5];
    //  printf("sensor[%u]\n", i);
    for (size_t j = 0; sensor[j]; j++) {
      if (j == 0 ) {
        strlcpy(objectId, sensor[j], sizeof(objectId));
      } else if (j == 1 ) {
        strlcpy(sensorId, sensor[j], sizeof(sensorId));
      }  else if (j == 2 ) {
        strlcpy(resourceId, sensor[j], sizeof(resourceId));
      }
    }
    if ( strcmp(message.sensorId, sensorId) == 0) {
      if ( strcmp(message.omaObjectId, "3349") == 0 && strcmp(message.omaResourceId, "5911") == 0) {
        if ( strcmp(message.method, "1") == 0 ) {
          if ( strcmp(message.payload, "true") == 0 ) {
            aSerial.vvvv().pln(F("ca-me-ra-me-raaaaaaaa"));
            return serverCapture(myCAM);
          }
        } else if ( strcmp(message.method, "4") == 0 ) {
          if ( strcmp(message.payload, "1") == 0 ) {
            transmitStream = true;
            return serverStream(myCAM);
          }
          if ( strcmp(message.payload, "0") == 0) {
            transmitStream = false;
            return;
          }
        }
      }
      else if ( ( strcmp(message.method, "1") == 0 ) && ( strcmp(message.omaObjectId, "2000") == 0 ) ) {
        if ( strcmp(message.omaResourceId, "reso") == 0 ) {
          //  int reso = atoi(message.payload);
          const char* reso = message.payload;
          aSerial.vv().p(F("Change resolution to : ")).pln(reso);
          if ( atoi(reso) >= 0 || atoi(reso) <= 8 ) {
            strlcpy(config.camResolution, reso,  sizeof(config.camResolution));
            setCamResolution(atoi(reso));
            //  return saveConfig(configFileName, config);
          }
        }
        if ( strcmp(message.omaResourceId, "fpm") == 0 ) {
          //  int fpm = atoi(message.payload);
          const char* fpm = message.payload;
          aSerial.vv().p(F("Change FPM to : ")).pln(fpm);
          if ( atoi(fpm) >= 0 || atoi(fpm) <= 4 ) {
            strlcpy(config.camFpm, fpm,  sizeof(config.camFpm));
            setFPM(atoi(fpm));
            //  return saveConfig(configFileName, config);
          }
        }
        if ( strcmp(message.omaResourceId, "update") == 0 ) {
          if (strcmp(message.payload, "ota") == 0) {
            //  strtok(s, "-");
            //  extract otaSignal, otaType, otaUrl, and fingerprint ( for httpsUpdate )
            //  if the int correspond to waited value, update otaSignal, otaFile and setReboot()
            //  return getUpdated();
          }

        }
      }
    }
    //    else {
    //      return;
    //    }
  }

}

void parseTopic(Message &message, char* topic) {
  char *str, *p;
  uint8_t i = 0;

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
          strlcpy(message.omaResourceId, str, sizeof(message.omaResourceId));
          break;
        }
    }
    i++;
  }
}
