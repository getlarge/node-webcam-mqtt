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
  if (temp != 0x55){
    Serial.println(F("SPI1 interface Error!"));
    while(1);
  }

  //Check if the camera module type is OV2640
  myCAM.wrSensorReg8_8(0xff, 0x01);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
   if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 )))
    Serial.println("Can't find OV2640 module! pid: " + String(pid));
   // Serial.println("Can't find OV2640 module!");
    else
    Serial.println(F("OV2640 detected."));
 
  //Change to JPEG capture mode and initialize the OV2640 module
  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  setFPM(fpm);
  myCAM.OV2640_set_JPEG_size(OV2640_320x240);
    //setCamResolution(resolution);
  myCAM.clear_fifo_flag();

//  Dir dir = SPIFFS.openDir("/pics");
//  while (dir.next()) {
//    fileCount++;
//  }
//
//  FSInfo fs_info;
//  SPIFFS.info(fs_info);
//
//  fileTotalKB = (int)fs_info.totalBytes;
//  fileUsedKB = (int)fs_info.usedBytes;
}

void start_capture(){
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
}

void camCapture(ArduCAM myCAM){
  #if WEB_SERVER == 1
  WiFiClient client = server.client();
  #endif
  digitalWrite(BUILTIN_LED, LOW);

  size_t len = myCAM.read_fifo_length();
  if (len >= 0x07ffff){
    Serial.println("Over size.");
    return;
  }else if (len == 0 ){
    Serial.println("Size is 0.");
    return;
  }
  
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
  #if !(defined (ARDUCAM_SHIELD_V2) && defined (OV2640_CAM))
  SPI.transfer(0xFF);
  #endif

  #if WEB_SERVER == 1
  if (!client.connected()) return;
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: image/jpeg\r\n";
    response += "Content-Length: " + String(len) + "\r\n\r\n";
    server.sendContent(response);
  #endif
  
  //static const size_t bufferSize = 2048;
  static uint8_t buffer[bufferSize] = {0xFF};

  while (len) {
      size_t will_copy = (len < bufferSize) ? len : bufferSize;
      SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
      #if WEB_SERVER == 1
        if (!client.connected()) break;
        client.write(&buffer[0], will_copy);
      #endif
      if ( base64.encode(&buffer[0], will_copy) == RBASE64_STATUS_OK) {
         //Serial.println(base64.result());
         mqttClient.publish(mqttTopicOut, base64.result(), sizeof(base64.result()));
      }
      //mqttClient.publish(mqttTopicOut, &buffer[0], will_copy);
      len -= will_copy;
      #if DEBUG_PAYLOAD == 1
        Serial.print(F("Publish buffer: "));
        Serial.write((const uint8_t *)&buffer[0], sizeof(will_copy));
        Serial.write(&buffer[0], will_copy);
      #endif
    }
  //if (!mqttClient.connected()) break;
  mqttClient.publish(mqttTopicOut, "eof");
  myCAM.CS_HIGH();
}

void serverCapture(){
  start_capture();
  Serial.println("CAM Capturing");

  int total_time = 0;

  total_time = millis();
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  total_time = millis() - total_time;
  Serial.print("capture total_time used (in miliseconds):");
  Serial.println(total_time, DEC);
  
  total_time = 0;
  
  Serial.println("CAM Capture Done!");
  total_time = millis();
  camCapture(myCAM);
  total_time = millis() - total_time;
  Serial.print("send total_time used (in miliseconds):");
  Serial.println(total_time, DEC);
  Serial.println("CAM send Done!");
}

void serverStream(){
  #if WEB_SERVER == 1
    WiFiClient client = server.client();
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
    server.sendContent(response);
  #endif
  
  while (1){
    start_capture();
    
    while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
    
    size_t len = myCAM.read_fifo_length();
    if (len >= 0x07ffff){
      Serial.println("Over size.");
      continue;
    }else if (len == 0 ){
      Serial.println("Size is 0.");
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
    #endif
    
    //static const size_t bufferSize = 2048;
    static uint8_t buffer[bufferSize] = {0xFF};
    
    while (len) {
      size_t will_copy = (len < bufferSize) ? len : bufferSize;
      SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
      #if WEB_SERVER == 1
        if (!client.connected()) break;
        client.write(&buffer[0], will_copy);
      #endif
      len -= will_copy;
    }
    myCAM.CS_HIGH();
    
  }
}

#if WEB_SERVER == 1
void handleNotFound(){
  String message = "Server is running!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(200, "text/plain", message);
  
  if (server.hasArg("ql")){
    int ql = server.arg("ql").toInt();
    myCAM.OV2640_set_JPEG_size(ql);delay(1000);
    Serial.println("QL change to: " + server.arg("ql"));
  }
}
#endif
