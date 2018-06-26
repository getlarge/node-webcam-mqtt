void checkConfig() {
  SPIFFS.begin();
  delay(10);
  // check for properties file
  File f = SPIFFS.open(fName, "r");
  if (!f ) {
// no file exists so lets format and create a properties file
    Serial.println("Please wait 30 secs for SPIFFS to be formatted");
    SPIFFS.format();
    Serial.println("Spiffs formatted");
    f = SPIFFS.open(fName, "w");
    if (!f) {
      Serial.println(F("Resolution file open failed"));
    }
    else {
      // write the defaults to the properties file
      Serial.println(F("====== Writing to resolution file ========="));
      f.println(resolution);
      f.close();
    }
  }
  else {
    // if the properties file exists on startup,  read it and set the defaults
    Serial.println(F("Resolution file exists. Reading."));
    while (f.available()) {
      // read line by line from the file
      String str = f.readStringUntil('\n');
      Serial.println(str);
      resolution = str.toInt();
    }
    f.close();
  }
  
  File f2 = SPIFFS.open(f2Name, "r");
  if (!f2) {
    // no file exists so lets format and create a properties file
//    Serial.println("Please wait 30 secs for SPIFFS to be formatted");
//    SPIFFS.format();
//    Serial.println("Spiffs formatted");
    f2 = SPIFFS.open(f2Name, "w");
    if (!f2) {
      Serial.println(F("FPM file open failed"));
    }
    else {
      // write the defaults to the properties file
      Serial.println(F("====== Writing to FPM file ========="));
      f2.println(fpm);
      f2.close();
    }
  }
  else {
    // if the properties file exists on startup,  read it and set the defaults
    Serial.println(F("FPM file exists. Reading."));

    while (f2.available()) {
      // read line by line from the file;
      String str2 = f2.readStringUntil('\n');
      Serial.println(str2);
      fpm = str2.toInt();
    }
    f2.close();
  }
}
///////////////////////////////////////////////////////////////////////////////////
//    used when form is submitted and at setup to set the camera resolution      //
///////////////////////////////////////////////////////////////////////////////////
void setCamResolution(int reso) {
  switch (reso) {
    case 0:
      myCAM.OV2640_set_JPEG_size(OV2640_160x120);
      resolution = 0;
      Serial.println(F("Resolution set to 160x120"));
      break;
    case 1:
      myCAM.OV2640_set_JPEG_size(OV2640_176x144);
      resolution = 1;
      Serial.println(F("Resolution set to 176x144"));
      break;
    case 2:
      myCAM.OV2640_set_JPEG_size(OV2640_320x240);
      resolution = 2;
      Serial.println(F("Resolution set to 320x240"));
      break;
    case 3:
      myCAM.OV2640_set_JPEG_size(OV2640_352x288);
      resolution = 3;
      Serial.println(F("Resolution set to 352x288"));
      break;
    case 4:
      myCAM.OV2640_set_JPEG_size(OV2640_640x480);
      resolution = 4;
      Serial.println(F("Resolution set to 640x480"));
      break;
    case 5:
      myCAM.OV2640_set_JPEG_size(OV2640_800x600);
      resolution = 5;
      Serial.println(F("Resolution set to 800x600"));
      break;
    case 6:
      myCAM.OV2640_set_JPEG_size(OV2640_1024x768);
      resolution = 6;
      Serial.println(F("Resolution set to 1024x768"));
      break;
    case 7:
      myCAM.OV2640_set_JPEG_size(OV2640_1280x1024);
      resolution = 7;
      Serial.println(F("Resolution set to 1280x1024"));
      break;
    case 8:
      myCAM.OV2640_set_JPEG_size(OV2640_1600x1200);
      resolution = 8;
      Serial.println(F("Resolution set to 1600x1200"));
      break;

  }
}

void setFPM(int interv) {
  switch (interv) {
    case 0:
      minDelayBetweenframes = 5000;
      fpm = 0;
      Serial.println(F("FPM set to 12"));
      break;
    case 1:
      minDelayBetweenframes = (10*1000);
      fpm = 1;
      Serial.println(F("FPM set to 6"));
      break;
    case 2:
      minDelayBetweenframes = (15*1000);
      fpm = 2;
      Serial.println(F("FPM set to 4"));
      break;
    case 3:
      minDelayBetweenframes = (30*1000);
      fpm = 3;
      Serial.println(F("FPM set to 2"));
      break;
    case 4:
      minDelayBetweenframes = (60*1000);
      fpm = 4;
      Serial.println(F("FPM set to 1"));
      break;
//    case 5:
//      minDelayBetweenframes = (12*1000);
//      fpm = 5;
//      Serial.println(F("FPM set to 5"));
//      break;
//    case 6:
//      minDelayBetweenframes = (10*1000);
//      fpm = 6;
//      Serial.println(F("FPM set to 6"));
//      break;
//    case 12:
//      minDelayBetweenframes = (5*1000);
//      fpm = 12;
//      Serial.println(F("FPM set to 12"));
//      break;
  }
}

/////////////////////////////////////////////////
//   Updates Properties file with resolution  ///
/////////////////////////////////////////////////
void updateResolutionFile() {
  File f = SPIFFS.open(fName, "w");
  if (!f) {
    Serial.println(F("Resolution file open failed"));
  }
  else
  {
    Serial.println(F("====== Writing to resolution file ========="));

    f.println(resolution);
    Serial.println(F("Resolution file updated"));
    f.close();
  }
}

void updateFPMFile() {
  File f2 = SPIFFS.open(f2Name, "w");
  if (!f2) {
    Serial.println(F("FPM file open failed"));
  }
  else
  {
    Serial.println(F("====== Writing to FPM file ========="));
    f2.println(fpm);
    Serial.println(F("FPM file updated"));
    f2.close();
  }
}
