///////////////////////////////////////////////////////////////////////////////////
//    functions to interact with MCU, FileSystem, physical world      //
///////////////////////////////////////////////////////////////////////////////////

void checkSerial() {
  while (Serial.available() == 0 && millis() < 4000);
  //On timeout or availability of data, we come here.
  if (Serial.available() > 0) {
    //If data is available, we enter here.
    int test = Serial.read(); //We then clear the input buffer
    Serial.println("DEBUG"); //Give feedback indicating mode
    Serial.println(DEBUG);
    aSerial.on();
  }
  else {
    aSerial.off();
    //Serial.setDebugOutput(false);
  }
}

/// FILE MANAGER

void updateFile(const String fileName, int value) {
  File f = SPIFFS.open(fileName.c_str(), "w");
  if (!f) {
    aSerial.vv().p(fileName.c_str()).pln(F("opening failed"));
  }
  else {
    aSerial.vvv().p("Writing to ").pln(fileName.c_str());
    f.println(value);
    aSerial.vvv().p(fileName.c_str()).pln(F(" updated"));
    f.close();
  }
}

void loadConfig(const String fileName, Config &config) {
  aSerial.vvv().pln(F("Reading config file."));
  File configFile = SPIFFS.open(fileName, "r");
  if (configFile) {
    size_t size = configFile.size();
    // Allocate a buffer to store contents of the file.
    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);
    //  StaticJsonDocument<(objBufferSize * 2)> obj;
    DynamicJsonDocument obj(objBufferSize * 2);
    //  DeserializationError error = deserializeJson(obj, buf.get());
    DeserializationError error = deserializeJson(obj, buf.get(), size);
    if (error) {
      aSerial.vv().p(F("Failed to load json config : ")).pln(error.c_str());
    }
    else {
      serializeJsonPretty(obj, Serial);
      strlcpy(config.mqttServer, obj["mqttServer"] | defaultMqttServer, sizeof(config.mqttServer));
      strlcpy(config.mqttPort, obj["mqttPort"] | defaultMqttPort, sizeof(config.mqttPort));
      strlcpy(config.mqttUser, obj["mqttUser"] | defaultMqttUser, sizeof(config.mqttUser));
      strlcpy(config.mqttPassword, obj["mqttPassword"] | defaultMqttPassword, sizeof(config.mqttPassword));
      strlcpy(config.mqttTopicIn, obj["mqttTopicIn"] | defaultMqttTopicIn,  sizeof(config.mqttTopicIn));
      strlcpy(config.mqttTopicOut, obj["mqttTopicOut"] | defaultMqttTopicOut,  sizeof(config.mqttTopicOut));
      if (obj["ip"]) {
        strcpy(config.staticIp, obj["ip"]);
        strcpy(config.staticGw, obj["gateway"]);
        strcpy(config.staticSn, obj["subnet"]);
      }
      if (obj["camResolution"]) {
        strlcpy(config.camResolution, obj["camResolution"],  sizeof(config.camResolution));
      }
      if (obj["camFpm"]) {
        strlcpy(config.camFpm, obj["camFpm"],  sizeof(config.camFpm));
      }
    }
  }
  else {
    aSerial.vv().pln(F("Failed to load config file."));
  }
  ticker.detach();
}

void saveConfig(const String fileName, Config &config) {
  strlcpy(config.mqttTopicIn, config.devEui, sizeof(config.mqttTopicIn));
  strcat(config.mqttTopicIn, config.inPrefix);
  strlcpy(config.mqttTopicOut, config.devEui, sizeof(config.mqttTopicOut));
  strcat(config.mqttTopicOut, config.outPrefix);
  //  StaticJsonDocument<objBufferSize> obj;
  DynamicJsonDocument obj(objBufferSize * 2);
  obj["mqttServer"] = config.mqttServer;
  obj["mqttPort"] = config.mqttPort;
  obj["mqttClient"] = config.mqttClient;
  obj["mqttUser"] = config.mqttUser;
  obj["mqttPassword"] = config.mqttPassword;
  obj["mqttTopicIn"] = config.mqttTopicIn;
  obj["mqttTopicOut"] = config.mqttTopicOut;
  obj["camResolution"] = config.camResolution;
  obj["camFpm"] = config.camFpm;
  //    obj["ip"] = WiFi.localIP().toString();
  //    obj["gateway"] = WiFi.gatewayIP().toString();
  //    obj["subnet"] = WiFi.subnetMask().toString();
  File configFile = SPIFFS.open(fileName, "w");
  if (!configFile) {
    aSerial.vv().pln(F("Failed to open config file"));
  }
#if DEBUG != 0
  if (serializeJsonPretty(obj, Serial) == 0) {
    Serial.println(F("Failed to write to Serial"));
  }
#endif
  if (serializeJson(obj, configFile) == 0) {
    Serial.println(F("Failed to write to file"));
  }
  configFile.close();
}

void initDefaultConfig(const String fileName, Config &config) {
  DynamicJsonDocument obj(objBufferSize * 2);
  obj["mqttServer"] = defaultMqttServer;
  obj["mqttPort"] = defaultMqttPort;
  obj["mqttClient"] = config.mqttClient;
  obj["mqttUser"] = defaultMqttUser;
  obj["mqttPassword"] = defaultMqttPassword;
  obj["mqttTopicIn"] = defaultMqttTopicIn;
  obj["mqttTopicOut"] = defaultMqttTopicOut;
  obj["camResolution"] = config.camResolution;
  obj["camFpm"] = config.camFpm;
  File configFile = SPIFFS.open(fileName, "w");
  if (!configFile) {
    aSerial.vv().pln(F("Failed to open config file"));
  }
#if DEBUG != 0
  if (serializeJsonPretty(obj, Serial) == 0) {
    Serial.println(F("Failed to write to Serial"));
  }
#endif
  if (serializeJson(obj, configFile) == 0) {
    Serial.println(F("Failed to write to file"));
  }
  configFile.close();
  strlcpy(config.mqttServer, defaultMqttServer, sizeof(config.mqttServer));
  strlcpy(config.mqttPort, defaultMqttPort, sizeof(config.mqttPort));
  strlcpy(config.mqttUser, defaultMqttUser, sizeof(config.mqttUser));
  strlcpy(config.mqttPassword, defaultMqttPassword, sizeof(config.mqttPassword));
  strlcpy(config.mqttTopicIn, defaultMqttTopicIn,  sizeof(config.mqttTopicIn));
  strlcpy(config.mqttTopicOut,  defaultMqttTopicOut,  sizeof(config.mqttTopicOut));
  strlcpy(config.mqttTopicIn, config.devEui, sizeof(config.mqttTopicIn));
  strcat(config.mqttTopicIn, config.inPrefix);
  strlcpy(config.mqttTopicOut, config.devEui, sizeof(config.mqttTopicOut));
  strcat(config.mqttTopicOut, config.outPrefix);
}

void tick() {
  int state = digitalRead(STATE_LED);
  digitalWrite(STATE_LED, !state);
}

void setPins() {
  pinMode(STATE_LED, OUTPUT);
  digitalWrite(STATE_LED, HIGH);
  debouncer.attach(OTA_BUTTON_PIN, INPUT_PULLUP);
  debouncer.interval(debouncerInterval);
  aSerial.vvv().pln(F("Pins set"));
}

void checkButton() {
  // Get the update value
  int value = debouncer.read();
  if ( value == HIGH) {
    buttonState = 0;
    aSerial.vvv().pln(F("Button released"));
  } else {
    buttonState = 1;
    aSerial.vvv().pln(F("Long push detected --> config mode"));
    buttonPressTimeStamp = millis();
  }
}

void setReboot() { // Boot to sketch
  pinMode(STATE_LED, OUTPUT);
  digitalWrite(STATE_LED, HIGH);
  pinMode(D8, OUTPUT);
  digitalWrite(D8, LOW);
  aSerial.vv().pln(F("Pins set for reboot..."));
  Serial.flush();
  yield();
  delay(5000);
  aSerial.v().println(F("====== Reboot ======"));
  ESP.reset(); //ESP.restart();
  delay(2000);
}

void setDefault() {
  ticker.attach(2, tick);
  aSerial.v().println(F("====== Reset config ======"));
  resetConfig = false;
  SPIFFS.begin();
  delay(10);
  SPIFFS.format();
  wifiManager.resetSettings();
  delay(100);
  aSerial.v().println(F("====== System cleared ======"));
  ticker.detach();
  aSerial.v().pln(ESP.eraseConfig());
  setReboot();
}

void getDeviceId(Config &config) {
#if ID_TYPE == 0
  char *espChipId;
  float chipId = ESP.getChipId();
  char chipIdBuffer[sizeof(chipId)];
  espChipId = dtostrf(chipId, sizeof(chipId), 0, chipIdBuffer);
  strcpy(config.devEui, espChipId);
#endif
#if ID_TYPE == 1
  String macAdress = WiFi.macAddress();
  char macAdressBuffer[20];
  macAdress.toCharArray(macAdressBuffer, 20);
  // next => remove the ":" in the mac adress
  strcpy(config.devEui, macAdressBuffer);
#endif
  //    #if ID_TYPE == 2
  //// soyons fous, let's create an eui64 address ( like ipv6 )
  ////      Step #1: Split the MAC address in the middle:
  ////      Step #2: Insert FF:FE in the middle:
  ////      Step #4: Convert the first eight bits to binary:
  ////      Step #5: Flip the 7th bit:
  ////      Step #6: Convert these first eight bits back into hex:
  //    #endif
  aSerial.vvv().p(F("DeviceID : ")).pln(config.devEui);
  generateMqttClientId(config);
}

void connectWifi() {
  String ssid = WiFi.SSID();
  String pass = WiFi.psk();
  //  IPAddress _ip, _gw, _sn;
  //  _ip.fromString(config.staticIp);
  //  _gw.fromString(config.staticGw);
  //  _sn.fromString(config.staticSn);
  WiFi.mode(WIFI_STA);
  if ((strcmp(defaultWifiSSID, "") != 0) && (strcmp(defaultWifiPass, "") != 0)) {
    WiFi.begin(defaultWifiSSID, defaultWifiPass);
  } else {
    WiFi.begin(ssid.c_str(), pass.c_str());
  }
  wifiFailCount = 0;
  String hostname(config.deviceName);
  WiFi.hostname(hostname);
  while (WiFi.status() != WL_CONNECTED) {
    aSerial.vvv().pln(F("Attempting Wifi connection...."));
    wifiFailCount += 1;
    if (wifiFailCount > 10 ) {
      return configManager(config);
    }
    delay(reconnectInterval);
  }
  aSerial.vv().p(F("WiFi connected. IP Address : ")).pln(WiFi.localIP());
}

/// OTA
void getUpdated(int which, const char* url) {
  if ((WiFi.status() == WL_CONNECTED)) {
    ticker.attach(0.7, tick);
    otaSignal = 0;
    updateFile(otaFile, otaSignal);
    ESPhttpUpdate.rebootOnUpdate(true);
    //  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

    t_httpUpdate_return ret;
    //    if ( which == 0 ) {
    //      WiFiClient client;
    //      aSerial.v().pln(F("Update Sketch..."));
    //      t_httpUpdate_return ret = ESPhttpUpdate.update(client, url);
    //    }
    //
    //    ticker.detach();
    //    switch (ret) {
    //      case HTTP_UPDATE_FAILED:
    //        aSerial.v().p(F("HTTP_UPDATE_FAILD Error : ")).p(ESPhttpUpdate.getLastError()).p(" / ").pln(ESPhttpUpdate.getLastErrorString().c_str());
    //        break;
    //      case HTTP_UPDATE_NO_UPDATES:
    //        aSerial.v().pln(F("HTTP_UPDATE_NO_UPDATES "));
    //        break;
    //      case HTTP_UPDATE_OK:
    //        aSerial.v().pln(F("HTTP_UPDATE_OK"));
    //        break;
    //    }
  }
}

/// TIME
#if NTP_SERVER == 1
void digitalClockDisplay() {
  aSerial.vvv().p(hour());
  printDigits(minute());
  printDigits(second());
  aSerial.vvv().p("").p(day()).p(".").p(month()).p(".").pln(year());
}

void printDigits(int digits) {
  // utility for digital clock display: prints preceding colon and leading 0
  aSerial.vvv().p(":");
  if (digits < 10)
    aSerial.vvv().p('0');
  aSerial.vvv().p(digits);
}

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
  IPAddress ntpServerIP; // NTP server's ip address
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  aSerial.vv().pln(F("Transmit NTP Request"));
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  aSerial.vv().p(ntpServerName).p(" : ").pln(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      aSerial.vvv().pln(F("Receiving NTP packet ..."));
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  aSerial.vv().pln(F("No NTP response"));
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address) {
  aSerial.vvv().pln(F("Sending NTP packet ..."));
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
#endif
