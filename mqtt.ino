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
void mqttReconnect(Config &config) {
  // Loop until we're reconnected
  mqttFailCount = 0;
  while (!mqttClient.connected()) {
    ++mqttFailCount;
    aSerial.vvv().p(F("Connecting to mqtt://")).p((const char*)config.mqttServer).p(":").pln(atoi(config.mqttPort));
    generateMqttClientId(config);
    if (mqttClient.connect(((const char*)config.mqttClient), ((const char*)config.mqttUser), ((const char*)config.mqttPassword))) {
      aSerial.vvv().p(F("Connected to mqtt as : ")).pln((const char*)config.mqttClient);
      char masterTopic[60];
      strlcpy(masterTopic, config.mqttTopicIn, sizeof(masterTopic));
      strcat(masterTopic, "/+/+/+/+" );
      mqttClient.subscribe((const char*)masterTopic);
      return presentSensors(config);
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

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  message.payload = ((char*)payload);
  aSerial.v().println(F("====== Received message ======"));
  aSerial.vv().p(F("Topic : ")).pln((const char*)topic).p(F("Payload : ")).pln((const char*)message.payload);
  parseTopic(message, topic);
  aSerial.vvv().p(F("Method : ")).pln((const char*)message.method).p(F("For sensor : ")).pln((const char*)message.omaObjectId);
  parseMessage(message);
}
