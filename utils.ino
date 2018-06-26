void tick() {
  int state = digitalRead(STATE_LED); 
  digitalWrite(STATE_LED, !state); 
}

void setPins(){
    pinMode(STATE_LED, OUTPUT);
    digitalWrite(STATE_LED, HIGH);
    pinMode(OTA_BUTTON_PIN, INPUT_PULLUP);
    debouncer.attach(OTA_BUTTON_PIN);
    debouncer.interval(3000);
    Serial.println(F("Pins set"));
}

void setReboot() { // Boot to sketch
//    pinMode(STATE_LED, OUTPUT);
//    digitalWrite(STATE_LED, HIGH);
//    pinMode(D8, OUTPUT);
//    digitalWrite(D8, LOW);
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

void checkButton(int context) {
  if ( context = 0 ) {
    debouncer.update();
    int value = debouncer.read();
    if (value == LOW) {
        Serial.println(F("Long push detected, asked for config"));
        manualConfig = true;
        configManager();
        value == HIGH;
    }
  }
  if ( context = 1 ) {
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

char* getDeviceId() {
//  String clientId = "Arducam-";
//  clientId += String(random(0xffff), HEX);  
  char msgBuffer[7];         
  char *espChipId;
  float chipId = ESP.getChipId();
  char deviceId[20];
  espChipId = dtostrf(chipId, 7, 0, msgBuffer);
  strcpy(deviceId,devicePrefix); 
  strcat(deviceId,espChipId);
  Serial.println(deviceId);
  return deviceId;
}

/// FILE MANAGER

void checkState() {
  SPIFFS.begin();
  delay(10);
  // check for properties file
  File f = SPIFFS.open(otaFile, "r");
//  File f1 = SPIFFS.open(fpmFile, "r");
//  File f2 = SPIFFS.open(resFile, "r");
  if (!f ) {
  // one of the config file  doesnt exist so lets format and create a properties file
    Serial.println("Please wait 30 secs for SPIFFS to be formatted");
    SPIFFS.format();
    Serial.println("Spiffs formatted");
  }
  else {
    f.close(); //f1.close(); f2.close();
  }
}

void checkFile(const String fileName, int value) {
  SPIFFS.begin();
  delay(10);
  // check for properties file
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
    Serial.printf("%s exists. Reading. \n", fileName.c_str());
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

/// CAM SETTINGS
void setCamResolution(int reso) {
  resolution = reso;
  updateFile(resFile, reso);
  switch (reso) {
    case 0:
      myCAM.OV2640_set_JPEG_size(OV2640_160x120);
      Serial.println(F("Resolution set to 160x120"));
      break;
    case 1:
      myCAM.OV2640_set_JPEG_size(OV2640_176x144);
      Serial.println(F("Resolution set to 176x144"));
      break;
    case 2:
      myCAM.OV2640_set_JPEG_size(OV2640_320x240);
      Serial.println(F("Resolution set to 320x240"));
      break;
    case 3:
      myCAM.OV2640_set_JPEG_size(OV2640_352x288);
      Serial.println(F("Resolution set to 352x288"));
      break;
    case 4:
      myCAM.OV2640_set_JPEG_size(OV2640_640x480);
      Serial.println(F("Resolution set to 640x480"));
      break;
    case 5:
      myCAM.OV2640_set_JPEG_size(OV2640_800x600);
      Serial.println(F("Resolution set to 800x600"));
      break;
    case 6:
      myCAM.OV2640_set_JPEG_size(OV2640_1024x768);
      Serial.println(F("Resolution set to 1024x768"));
      break;
    case 7:
      myCAM.OV2640_set_JPEG_size(OV2640_1280x1024);
      Serial.println(F("Resolution set to 1280x1024"));
      break;
    case 8:
      myCAM.OV2640_set_JPEG_size(OV2640_1600x1200);
      Serial.println(F("Resolution set to 1600x1200"));
      break;
  }
}

void setFPM(int interv) {
  fpm = interv;
  Serial.printf("FPM set to %i. \n", interv);
  updateFile(fpmFile, interv);
  switch (interv) {
    case 0:
      minDelayBetweenframes = 5000;
      break;
    case 1:
      minDelayBetweenframes = (10*1000);
      break;
    case 2:
      minDelayBetweenframes = (15*1000);
      break;
    case 3:
      minDelayBetweenframes = (30*1000);
      break;
    case 4:
      minDelayBetweenframes = (60*1000);
      break;
//    case 5:
//      minDelayBetweenframes = (12*1000);
//      break;
//    case 6:
//      minDelayBetweenframes = (10*1000);

//      break;
//    case 12:
//      minDelayBetweenframes = (5*1000);
//      break;
  }
}

/// TIME
void digitalClockDisplay() {
  // digital clock display of the time
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
/*-------- NTP code ----------*/

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

