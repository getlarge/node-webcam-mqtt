///////////////////////////////////////////////////////////////////////////////////
//    function to interact with MCU, FileSystem, physical world      //
///////////////////////////////////////////////////////////////////////////////////

void tick() {
  int state = digitalRead(STATE_LED);
  digitalWrite(STATE_LED, !state);
}

void setPins() {
  pinMode(STATE_LED, OUTPUT);
  digitalWrite(STATE_LED, HIGH);
  pinMode(OTA_BUTTON_PIN, INPUT_PULLUP);
  debouncer.attach(OTA_BUTTON_PIN);
  debouncer.interval(3000);
  Serial.println(F("Pins set"));
}

void checkButton(int context) {
  if ( context == 0 ) {
    debouncer.update();
    int value = debouncer.read();
    if (value == LOW) {
      Serial.println(F("Long push detected, asked for config"));
      manualConfig = true;
      configManager();
      value == HIGH;
    }
  }
  if ( context == 1 ) {
    debouncer.update();
    int value = debouncer.read();
    if (value == LOW) {
      Serial.println(F("Long push detected, asked for return"));
      manualConfig = false;
      configManager();
      value == HIGH;
      return;
    }
  }
}

void setReboot() { // Boot to sketch
  pinMode(STATE_LED, OUTPUT);
  digitalWrite(STATE_LED, HIGH);
  pinMode(D8, OUTPUT);
  digitalWrite(D8, LOW);
  Serial.println(F("Pins set for reboot"));
  //    Serial.flush();
  //    yield(); yield(); delay(500);
  delay(5000);
  ESP.reset(); //ESP.restart();
  delay(2000);
}

void setDefault() {
  ticker.attach(2, tick);
  Serial.println(F("Resetting config to the inital state"));
  resetConfig = false;
  SPIFFS.begin();
  delay(10);
  SPIFFS.format();
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  delay(100);
  Serial.println(F("System cleared"));
  ticker.detach();
  Serial.println(ESP.eraseConfig());
  setReboot();
}

void getDeviceId() {
  #if ID_TYPE == 0
    char deviceId[20];
    char *espChipId;
    float chipId = ESP.getChipId();
    char chipIdBuffer[sizeof(chipId)];
    espChipId = dtostrf(chipId, sizeof(chipId), 0, chipIdBuffer);
    strcpy(deviceId, devicePrefix);
    strcat(deviceId, espChipId);
  #endif
  #if ID_TYPE == 1
    char deviceId[20];
    String macAdress = WiFi.macAddress();
    char macAdressBuffer[20];
    macAdress.toCharArray(macAdressBuffer, 20);
    // next => remove the ":" in the mac adress
    strcpy(deviceId,devicePrefix);
    strcat(deviceId,macAdressBuffer);
  #endif
//    #if ID_TYPE == 2
//// soyons fous, let's create an eui64 address ( like ipv6 )
////      Step #1: Split the MAC address in the middle:
////      Step #2: Insert FF:FE in the middle:
////      Step #4: Convert the first eight bits to binary:
////      Step #5: Flip the 7th bit:
////      Step #6: Convert these first eight bits back into hex:
//    #endif
  strcpy(config.mqtt_client, deviceId);
  delay(100); 
  Serial.println(deviceId);
}

/// FILE MANAGER
void checkState() {
  SPIFFS.begin();
  delay(10);
  // check for properties file
  File f = SPIFFS.open(otaFile, "r");
  if (!f ) {
    // one of the config file  doesnt exist so lets format and create a properties file
    //    Serial.println("Please wait 30 secs for SPIFFS to be formatted");
    //    SPIFFS.format();
    //    Serial.println("Spiffs formatted");
    //    setReboot();
  }
  else {
    f.close(); //f1.close(); f2.close();
  }
}

void checkFile(const String fileName, int value) {
  SPIFFS.begin();
  delay(10);
  File f = SPIFFS.open(fileName, "r");
  if (!f ) {
    f = SPIFFS.open(fileName, "w");
    if (!f) {
      Serial.printf("%s open failed \n", fileName.c_str());
    }
    else {
      Serial.printf("====== Writing to %s ========= \n", fileName.c_str());
      f.println(value);
      f.close();
    }
  }
  else {
    // if the properties file exists on startup,  read it and set the defaults
    Serial.printf("%s exists. Reading. ", fileName.c_str());
    while (f.available()) {
      if ( fileName == "ota.txt" ) {
        String str = f.readStringUntil('\n');
        Serial.println(str);
        // extract otaSignal, otaType, otaUrl, and fingerprint ( for httpsUpdate )
        // if the int correspond to waited value, update otaSignal, otaFile and setReboot()
        //value = str.toInt();
      }
      else {
        String str = f.readStringUntil('\n');
        Serial.println(str);
        value = str.toInt();
      }
    }
    f.close();
  }
}

void connectWifi() {

    /// use saved paramater, but you can hardcode yours here
    String ssid = WiFi.SSID();
    String pass = WiFi.psk();
    WiFi.begin(ssid.c_str(), pass.c_str());
   
    while (WiFi.status() != WL_CONNECTED) { 
       Serial.print("Attempting Wifi connection....");     
       delay(1000);    
    }
    Serial.println();
    Serial.print("WiFi connected.  IP address:");
    Serial.println(WiFi.localIP());    
}

void updateFile(const String fileName, int value) {
  File f = SPIFFS.open(fileName.c_str(), "w");
  if (!f) {
    Serial.printf("%s open failed \n", fileName.c_str());
  }
  else {
    Serial.printf("====== Writing to %s  ========= \n", fileName.c_str());
    f.println(value);
    Serial.printf(" %s  updated \n", fileName.c_str());
    f.close();
  }
}

/// OTA
void getUpdated(int which, const char* url, const char* fingerprint) {
  if ((WiFi.status() == WL_CONNECTED)) {
    ticker.attach(0.7, tick);
    otaSignal = 0;
    updateFile(otaFile, otaSignal);
    ESPhttpUpdate.rebootOnUpdate(true);
    t_httpUpdate_return ret;
    if ( which == 0 ) {
      Serial.println(F("Update Sketch..."));
      t_httpUpdate_return ret = ESPhttpUpdate.update(url);
    }
    if ( which == 1 ) {
      Serial.println(F("Update Sketch..."));
      //t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs(otaUrl, "", httpsFingerprint);
      t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs(url, "", fingerprint);
    }
    if ( which == 2 ) {
      Serial.println(F("Update SPIFFS..."));
      t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs(url);
    }
    if ( which == 3 ) {
      Serial.println(F("Update SPIFFS..."));
      t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs(url, "", fingerprint);
    }
    ticker.detach();
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

/// TIME
#if NTP_SERVER == 1
void digitalClockDisplay() {
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println();
}

void printDigits(int digits) {
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
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
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address)
{
  Serial.println(F("sending NTP packet..."));
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
