#include <FS.h> 

#include <ArduinoJson.h>  
//#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#if WEB_SERVER == 1
  #include <ESP8266WebServer.h>
  #include <ESP8266HTTPClient.h>        
#endif
#include <DNSServer.h>
#if NTP_SERVER == 1
  #include <WiFiUdp.h>
#endif
#include <WiFiManager.h>
#if WEB_SERVER_SECURE == 0 
  #include <WiFiClientSecure.h> 
#endif
#include <PubSubClient.h>
#include <rBase64.h>
#include <TimeLib.h>
#include <Ticker.h>
#include <Bounce2.h>
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>

#include "memorysaver.h" // config file in Arducam library
#include "config.h"
#if !(defined ESP8266 )
#error Please select the ArduCAM ESP8266 UNO board in the Tools/Board
#endif
#if NTP_SERVER == 1
  WiFiUDP Udp;
#endif
ESP8266WiFiMulti WiFiMulti;
#if CLIENT_SECURE == 1
  WiFiClientSecure wifiClient;
#endif
#if CLIENT_SECURE == 0
  WiFiClient wifiClient;
#endif
#if WEB_SERVER == 1
  ESP8266WebServer server(80);
#endif
PubSubClient mqttClient(wifiClient);
ArduCAM myCAM(OV2640, CS);
Bounce debouncer = Bounce();
Ticker ticker;
rBase64generic<camBufferSize> base64;
//rBase64generic<2048> base64;

mqttConfig config; 
messageFormat message; 

void tick();
void setPins();
void checkButton(int context);
void setReboot();
void setDefault();
void getDeviceId();
void checkState();
void checkFile(const String fileName, int value);
void updateFile(const String fileName, int value);
void getUpdated(int which, const char* url, const char* fingerprint);
void setPinsRebootUart();
void connectWifi();
void saveConfigCallback();
void configModeCallback (WiFiManager *myWiFiManager);
void configManager();
void arducamInit();
void setCamResolution(int reso);
void setFPM(int interv);
void serverCapture();
void serverStream();
void mqttInit();
void mqttError();
boolean mqttConnect();
void mqttCallback(char* topic, byte* payload, unsigned int length);
#if WEB_SERVER == 1
  void handleNotFound();
#endif
#if NTP_SERVER == 1
  time_t getNtpTime();
  time_t prevDisplay = 0; // when the digital clock was displayed
  void digitalClockDisplay();
  void printDigits(int digits);
  void sendNTPpacket(IPAddress &address);
#endif 

void before() {
  //checkState();
  checkFile(otaFile, otaSignal);  
  if (otaSignal == 1 ) {
     //WiFi.persistent(false);
     String ssid = WiFi.SSID();
     String pass = WiFi.psk();
     WiFiMulti.addAP(ssid.c_str(), pass.c_str());
     while (WiFiMulti.run() != WL_CONNECTED) { //use this when using ESP8266WiFiMulti.h
        Serial.println("Attempting Wifi connection.... ");     
        delay(500);    
    }
    Serial.print("WiFi connected.  IP address:");
    Serial.println(WiFi.localIP());  
    //getUpdated(int which, const char* url, const char* fingerprint) { 
  }
  for (uint8_t t = 4; t > 0; t--) { // Utile en cas d'OTA ?
      Serial.printf("[SETUP] WAIT %d...\n", t);
      Serial.flush();
      delay(1000);
  }
  setPins();
  ticker.attach(1.5, tick);
  if (wifiResetConfig) { // add a button ?
    WiFiManager wifiManager;
    wifiManager.resetSettings();
  }
  if (resetConfig) {
    setDefault(); 
  }
  Serial.printf("before heap size: %u\n", ESP.getFreeHeap());
}

void loadConfig() {
  Serial.println();
  Serial.println(F("mounting FS..."));
  if (SPIFFS.begin()) {
    Serial.println(F("mounted file system"));
    if (SPIFFS.exists("/config.json")) {
      Serial.println(F("reading config file"));
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println(F("opened config file"));
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        Serial.println(sizeof(buf.get()));
        // for ArduinoJson v6
        //StaticJsonDocument<(objBufferSize * 2)> doc;
//        DeserializationError error = deserializeJson(doc, buf.get());
//        if (error) {
//          Serial.println(F("Failed to load json config"));
//        }
//        JsonObject& obj = doc.as<JsonObject>();
//        if (serializeJsonPretty(doc, Serial) == 0) {
//          Serial.println(F("Failed to write to file"));
//        } 
        // for ArduinoJson v5
        StaticJsonBuffer<(objBufferSize * 2)> doc;
        JsonObject& obj = doc.parseObject(buf.get());
        obj.printTo(Serial);
        if (obj.success()) {
          strlcpy(config.mqtt_server, obj["mqtt_server"] | "server2.getlarge.eu", sizeof(config.mqtt_server));
          strlcpy(config.mqtt_port, obj["mqtt_port"] | "4006",  sizeof(config.mqtt_port));         
          strlcpy(config.mqtt_client, obj["mqtt_client"] | SKETCH_NAME,  sizeof(config.mqtt_client));         
          strlcpy(config.mqtt_user, obj["mqtt_user"] | "",  sizeof(config.mqtt_user));         
          strlcpy(config.mqtt_password, obj["mqtt_password"] | "",  sizeof(config.mqtt_password));
          strlcpy(config.mqtt_topic_in, obj["mqtt_topic_in"] | "node-webcam-in",  sizeof(config.mqtt_topic_in));
          strlcpy(config.mqtt_topic_out, obj["mqtt_topic_out"] | "node-webcam-out",  sizeof(config.mqtt_topic_out));
          strlcpy(captureTopic, config.mqtt_topic_out, sizeof(captureTopic));
          strcat(captureTopic, "/capture");
          Serial.println();
        }
        else {
          Serial.println(F("Failed to load json config"));
        }
      }
    }
  } else {
    Serial.println(F("Failed to mount FS"));
  }
  ticker.detach();
}

void setup() {
  Serial.begin(BAUD_RATE);
  #if DEBUG == 0
    Serial.setDebugOutput(false);
  #endif 
  Serial.println(F("====== Before setup ========="));
  before();
  Serial.println(F("====== Setup started ========="));
  loadConfig();
  checkFile(fpmFile, fpm);
  checkFile(resFile, resolution);
  arducamInit();
    getDeviceId();
    
  #if WEB_SERVER == 0
    configManager();
  #elif WEB_SERVER == 1
    connectWifi();
  #endif

  
  delay(1000);
  #if NTP_SERVER == 1
    Serial.println(F("Starting UDP"));
    Udp.begin(localPort);
    Serial.print("Local port: ");
    Serial.println(Udp.localPort());
    Serial.println(F("waiting for sync"));
    setSyncProvider(getNtpTime);
    setSyncInterval(300);
    delay(100);
  #endif
  #if WEB_SERVER == 1
    server.on("/capture", HTTP_GET, serverCapture);
    server.on("/stream", HTTP_GET, serverStream);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("Server started");
  #endif
  Serial.printf("setup heap size: %u\n", ESP.getFreeHeap());
  Serial.println(F("====== Setup ended========="));
}

void loop() {
  checkButton(0);

  if ( ! executeOnce) {
    executeOnce = true;
  }
//  if ( WiFi.status() != WL_CONNECTED) { // WiFiMulti.run() != WL_CONNECTED
//    ++wifiFailCount;
//    ticker.attach(0.5, tick);
//    if (wifiFailCount == 10) {
//      configManager();
//    }  
//  }
  #if WEB_SERVER == 0
  if (!mqttClient.connected()) {
    long now = millis();
    checkButton(0);
    ticker.attach(0.3, tick);
    if (now - lastMqttReconnectAttempt > reconnectInterval) {
      lastMqttReconnectAttempt = now;
      ++mqttFailCount;
      if (mqttConnect()) {
        mqttFailCount = 0;
        lastMqttReconnectAttempt = 0;
      }
      if (mqttFailCount > 5) {
        Serial.println(F("After 5 MQTT connection failure --> config mode"));
        lastMqttReconnectAttempt = 0;
        configManager();        
      }
    }
  }
  #endif

  else {
    #if WEB_SERVER == 0
      mqttClient.loop();
    #endif
    long now = millis();

    #if WEB_SERVER == 1
      server.handleClient();
    #endif
    
    #if NTP_SERVER == 1
      if (timeStatus() != timeNotSet) {
        if (now() != prevDisplay) { //update the display only if time has changed
          prevDisplay = now();
          digitalClockDisplay();
        }
      }
    #endif
    
    if (timelapse == true) {
      if ((now - lastPictureAttempt > minDelayBetweenframes) && transmitNow == true) {
        lastPictureAttempt = now;
        return serverCapture();
      }
    }
  }   
}
