///////////////////////////////////////////////////////////////////////////////////
//   handle arducam init, config, capture, and stream sending      //
///////////////////////////////////////////////////////////////////////////////////
void arducamInit() {
  uint8_t vid, pid;
  uint8_t temp;
#if defined(__SAM3X8E__)
  Wire1.begin();
#else
  Wire.begin();
#endif
  ticker.detach();
  digitalWrite(STATE_LED, HIGH);
  pinMode(CS, OUTPUT);
  SPI.begin();
  SPI.setFrequency(4000000); //4MHz
  //Check if the ArduCAM SPI bus is OK
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55) {
    aSerial.v().println(F("SPI1 interface Error!"));
    while (1);
  }

  //Check if the camera module type is OV2640
  myCAM.wrSensorReg8_8(0xff, 0x01);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 ))) {
    aSerial.v().print(F("Can't find OV2640 module! pid :")).println(String(pid));
  }
  else {
    aSerial.vvv().println(F("OV2640 detected."));
    myCAM.set_format(JPEG);
    //  myCAM.set_format(RAW);
    myCAM.InitCAM();
    //  setFPM(fpm);
    //  setBufferSize(camBufferSize);
    //  setCamResolution(resolution);
    setFPM(atoi(config.camFpm));
    setCamResolution(atoi(config.camResolution));
    myCAM.clear_fifo_flag();
  }
  //  Dir dir = SPIFFS.openDir("/pics");
  //  while (dir.next()) {
  //    fileCount++;
  //  }
  //  FSInfo fs_info;
  //  SPIFFS.info(fs_info);
  //  fileTotalKB = (int)fs_info.totalBytes;
  //  fileUsedKB = (int)fs_info.usedBytes;
}

/// CAM SETTINGS
void setCamResolution(int reso) {
  switch (reso) {
    case 0:
      myCAM.OV2640_set_JPEG_size(OV2640_160x120);
      aSerial.vvv().println(F("Resolution set to 160x120"));
      break;
    case 1:
      myCAM.OV2640_set_JPEG_size(OV2640_176x144);
      aSerial.vvv().println(F("Resolution set to 176x144"));
      break;
    case 2:
      myCAM.OV2640_set_JPEG_size(OV2640_320x240);
      aSerial.vvv().println(F("Resolution set to 320x240"));
      break;
    case 3:
      myCAM.OV2640_set_JPEG_size(OV2640_352x288);
      aSerial.vvv().println(F("Resolution set to 352x288"));
      break;
    case 4:
      myCAM.OV2640_set_JPEG_size(OV2640_640x480);
      aSerial.vvv().println(F("Resolution set to 640x480"));
      break;
    case 5:
      myCAM.OV2640_set_JPEG_size(OV2640_800x600);
      aSerial.vvv().println(F("Resolution set to 800x600"));
      break;
    case 6:
      myCAM.OV2640_set_JPEG_size(OV2640_1024x768);
      aSerial.vvv().println(F("Resolution set to 1024x768"));
      break;
    case 7:
      myCAM.OV2640_set_JPEG_size(OV2640_1280x1024);
      aSerial.vvv().println(F("Resolution set to 1280x1024"));
      break;
    case 8:
      myCAM.OV2640_set_JPEG_size(OV2640_1600x1200);
      aSerial.vvv().println(F("Resolution set to 1600x1200"));
      break;
  }
}

void setFPM(int interv) {
  aSerial.vvv().print(F("FPM set to : ")).println(interv);
  switch (interv) {
    case 0:
      minDelayBetweenframes = 1000;
      break;
    case 1:
      minDelayBetweenframes = (5 * 1000);
      break;
    case 2:
      minDelayBetweenframes = (10 * 1000);
      break;
    case 3:
      minDelayBetweenframes = (15 * 1000);
      break;
    case 4:
      minDelayBetweenframes = (30 * 1000);
      break;
  }
}

void start_capture() {
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
}

void camCapture(ArduCAM myCAM,  Message &message) {
#if WEB_SERVER == 1
  WiFiClient client = server.client();
#endif
  size_t len = myCAM.read_fifo_length();
  if (len >= 0x07ffff) {
    aSerial.vv().println(F("Over size."));
    return;
  } else if (len == 0 ) {
    aSerial.vv().println(F("Size is 0."));
    return;
  }
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
#if !(defined (ARDUCAM_SHIELD_V2) && defined (OV2640_CAM))
  SPI.transfer(0xFF);
#endif

  static uint8_t buffer[camBufferSize] = {0xFF};

#if WEB_SERVER == 1
  if (!client.connected()) return;
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: image/jpeg\r\n";
  response += "Content-Length: " + String(len) + "\r\n\r\n";
  server.sendContent(response);
#elif WEB_SERVER == 0
  if (!mqttClient.connected()) return;
  mqttClient.beginPublish((const char*)message.captureTopic, len, false);
#endif
  while (len) {
    size_t will_copy = (len < camBufferSize) ? len : camBufferSize;
    SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
#if WEB_SERVER == 0
    mqttClient.write(&buffer[0], will_copy);
#elif WEB_SERVER == 1
    client.write(&buffer[0], will_copy);
#endif
    len -= will_copy;
  }
#if WEB_SERVER == 0
  mqttClient.endPublish();
#endif
  myCAM.CS_HIGH();
}

void serverCapture(ArduCAM myCAM,  Message &message) {
  transmitNow = false;
  start_capture();
  digitalWrite(STATE_LED, LOW);
  aSerial.vv().println(F("Start Capturing"));
  int total_time = 0;
  total_time = millis();
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  total_time = millis() - total_time;
  aSerial.vvv().print(F("Capturing time (in ms) : ")).println(total_time, DEC);
  total_time = 0;
  aSerial.vv().println(F("Capture done!"));
  total_time = millis();
  camCapture(myCAM, message);
  total_time = millis() - total_time;
  aSerial.vvv().print(F("Sending time (in ms) : ")).println(total_time, DEC);
  aSerial.vvv().print(F("after capture heap size : ")).println(ESP.getFreeHeap());
  transmitNow = true;
  digitalWrite(STATE_LED, HIGH);
}

void serverStream(ArduCAM myCAM,  Message &message) {
  transmitNow = false;
#if WEB_SERVER == 1
  WiFiClient client = server.client();
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);
#endif
  while (1) {
    start_capture();
    while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
    size_t len = myCAM.read_fifo_length();
    if (len >= 0x07ffff) {
      aSerial.vv().println(F("Over size."));
      continue;
    }
    else if (len == 0 ) {
      aSerial.vv().println(F("Size is 0."));
      continue;
    }
    myCAM.CS_LOW();
    myCAM.set_fifo_burst();
#if !(defined (ARDUCAM_SHIELD_V2) && defined (OV2640_CAM))
    SPI.transfer(0xFF);
#endif
#if WEB_SERVER == 1
    if (!client.connected()) break;
    response = "--frame\r\n";
    response += "Content-Type: image/jpeg\r\n\r\n";
    server.sendContent(response);
#elif WEB_SERVER == 0
    if (!mqttClient.connected()) return;
    mqttClient.beginPublish((const char*)message.streamTopic, len, false);
#endif
    static uint8_t buffer[camBufferSize] = {0xFF};

    while (len) {
      size_t will_copy = (len < camBufferSize) ? len : camBufferSize;
      SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
#if WEB_SERVER == 0
      mqttClient.write(&buffer[0], will_copy);
#elif WEB_SERVER == 1
      client.write(&buffer[0], will_copy);
#endif
      len -= will_copy;
    }
#if WEB_SERVER == 0
    mqttClient.endPublish();
#endif
    myCAM.CS_HIGH();
  }
}

#if WEB_SERVER == 1
void handleNotFound() {
  String message = "Server is running!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(200, "text/plain", message);
  if (server.hasArg("ql")) {
    int ql = server.arg("ql").toInt();
    myCAM.OV2640_set_JPEG_size(ql); delay(1000);
    aSerial.vvv().print(F("QL changed to : ")).println(server.arg("ql"));
  }
}
#endif
