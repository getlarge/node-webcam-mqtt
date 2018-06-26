#include <FS.h> 

#include <ArduinoJson.h>  
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>        
#include <ESP8266httpUpdate.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiUdp.h>
#include <WiFiManager.h> 
#include <WiFiClientSecure.h> 
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

WiFiUDP Udp;
ESP8266WiFiMulti WiFiMulti;
#if CLIENT_SECURE == 1
  WiFiClientSecure MqttWifiClient;
#endif
#if CLIENT_SECURE == 0
  WiFiClient MqttWifiClient;
#endif
#if WEB_SERVER == 1
ESP8266WebServer server(80);
#endif
PubSubClient mqttClient(MqttWifiClient);
ArduCAM myCAM(OV2640, CS);
Bounce debouncer = Bounce();
Ticker ticker;
//rBase64generic<2048> base64;
rBase64generic<bufferSize> base64;

void tick();
void setPins();
void setReboot();
void setDefault();
void checkState();
void checkFile(const String fileName, int value);
void updateFile(const String fileName, int value);
void checkButton(int e);
char* getDeviceId();
void getUpdated(int which, const char* url, const char* fingerprint);
void connectWifi();
void setPinsRebootUart();
void saveConfigCallback();
void configManager();
void arducamInit();
void serverCapture();
void serverStream();
void setCamResolution(int reso);
void setFPM(int interv);
void mqttInit();
void mqttError();
boolean mqttConnect();
void mqttCallback(char* topic, byte* payload, unsigned int length);
#if WEB_SERVER == 1
  void handleNotFound();
#endif
time_t getNtpTime();
time_t prevDisplay = 0; // when the digital clock was displayed
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

void before() {
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
        DynamicJsonDocument doc;
        DeserializationError error = deserializeJson(doc, buf.get());
        if (error) {
          Serial.println(F("Failed to load json config"));
        }
        JsonObject& json = doc.as<JsonObject>();
        strcpy(mqtt_server, json["mqtt_server"]); strcpy(mqtt_port, json["mqtt_port"]);
        strcpy(mqtt_client_id, json["mqtt_client_id"]); strcpy(mqtt_user, json["mqtt_user"]); strcpy(mqtt_password, json["mqtt_password"]);
        //strcpy(http_server, json["http_server"]); strcpy(http_port, json["http_port"]);
        strcpy(mqtt_topic_in,mqtt_client_id); strcat(mqtt_topic_in,in); 
        strcpy(mqtt_topic_in1,mqtt_client_id); strcat(mqtt_topic_in1,in1);
        strcpy(mqtt_topic_in2,mqtt_client_id); strcat(mqtt_topic_in2,in2);
        strcpy(mqtt_topic_in3,mqtt_client_id); strcat(mqtt_topic_in3,in3);
        strcpy(mqtt_topic_in4,mqtt_client_id); strcat(mqtt_topic_in4,in4);
        strcpy(mqtt_topic_out,mqtt_client_id); strcat(mqtt_topic_out,out);
        strcpy(mqtt_topic_out1,mqtt_client_id); strcat(mqtt_topic_out1,out1);
        //strcpy(post_destination,post_prefix); strcat(post_destination,mqtt_client_id); 
        mqttServer = mqtt_server; mqttPort = atoi(mqtt_port);
        mqttClientId = mqtt_client_id; mqttUser = mqtt_user; mqttPassword = mqtt_password;
        mqttTopicIn = mqtt_topic_in; mqttTopicIn1 = mqtt_topic_in1; mqttTopicIn2 = mqtt_topic_in2; mqttTopicIn3 = mqtt_topic_in3; mqttTopicIn4 = mqtt_topic_in4;
        mqttTopicOut = mqtt_topic_out; mqttTopicOut1 = mqtt_topic_out1;
        //httpServer = http_server; httpPort = atoi(http_port); postDestination = post_destination;
        serializeJson(doc, Serial); 
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
  checkState();
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
  
  Serial.printf("before heap size: %u\n", ESP.getFreeHeap());
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
  getDeviceId();
  before();
  checkFile(fpmFile, fpm);
  checkFile(resFile, resolution);
  
  Serial.printf("setup heap size: %u\n", ESP.getFreeHeap());
  arducamInit();
  configManager();
  Serial.println(F("Starting UDP"));
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println(F("waiting for sync"));
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  mqttInit();
  
  #if WEB_SERVER == 1
    server.on("/capture", HTTP_GET, serverCapture);
    server.on("/stream", HTTP_GET, serverStream);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("Server started");
  #endif
}

void loop() {
  if ( ! executeOnce) {
    executeOnce = true;
  }
  #if WEB_SERVER == 1
    server.handleClient();
  #endif
  checkButton(0);
  
  if ( WiFi.status() != WL_CONNECTED) { // WiFiMulti.run() != WL_CONNECTED
    ++wifiFailCount;
    ticker.attach(0.1, tick);
    if (wifiFailCount == 10) {
      configManager();
    }  
  }
  if (!mqttClient.connected()) {
    ++mqttFailCount;
    checkButton(0);
    long now = millis();
    mqttError(2);
    if (now - lastMqttReconnectAttempt > interval1) {
        lastMqttReconnectAttempt = now;
        if (mqttFailCount == 5) {
          mqttError(3);
        }
        if (mqttConnect()) {
          lastMqttReconnectAttempt = 0;
        }
    }
  }
  else {
      if (timeStatus() != timeNotSet) {
        if (now() != prevDisplay) { //update the display only if time has changed
          prevDisplay = now();
          digitalClockDisplay();
        }
      }
      if (transmitNow) { // checks if the buffer is full
      }
      mqttClient.loop();
  }
    
}
