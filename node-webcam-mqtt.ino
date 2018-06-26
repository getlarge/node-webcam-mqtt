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
rBase64generic<2048> base64;

void tick();
void setPins();
void setReboot();
void setDefault();
void checkButton();
void getDeviceId();
void checkOtaFile();
void updateOtaFile();
void connectWifi();
void getUpdated();
void setPinsRebootUart();
void saveConfigCallback();
time_t getNtpTime();
time_t prevDisplay = 0; // when the digital clock was displayed
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

void arducamInit();
void serverCapture();
void serverStream();
void handleNotFound();
//void sendPic(int len);
void setCamResolution(int reso);
void setFPM(int interv);
void updateResolutionFile();
void updateFPMFile();
void checkConfig();
void configManager();
void mqttInit();
boolean mqttConnect();
bool mqttReconnect();
void mqttCallback(char* topic, byte* payload, unsigned int length);


void before() {
  Serial.println();
  Serial.println(F("mounting FS..."));
  if (SPIFFS.begin()) {
    Serial.println(F("mounted file system"));
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
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
        strcpy(mqtt_server, json["mqtt_server"]);
        strcpy(mqtt_port, json["mqtt_port"]);
        strcpy(mqtt_client_id, json["mqtt_client_id"]);
        strcpy(mqtt_user, json["mqtt_user"]);
        strcpy(mqtt_password, json["mqtt_password"]);
        strcpy(http_server, json["http_server"]);
        strcpy(http_port, json["http_port"]);
        strcpy(mqtt_topic_in,mqtt_client_id); 
        strcat(mqtt_topic_in,in); 
        strcpy(mqtt_topic_out,mqtt_client_id);
        strcat(mqtt_topic_out,out);
        strcpy(post_destination,post_prefix);
        strcat(post_destination,mqtt_client_id); 
        mqttServer = mqtt_server;
        mqttPort = atoi(mqtt_port);
        //mqttClientId = mqtt_client_id;
        mqttClientId = deviceId;
        mqttUser = mqtt_user;
        mqttPassword = mqtt_password;
        mqttTopicIn = mqtt_topic_in;
        mqttTopicOut = mqtt_topic_out;
        httpServer = http_server;
        httpPort = atoi(http_port);
        postDestination = post_destination;
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
  checkOtaFile();
  delay(100);
  if (_otaSignal == 1 ) {
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
    getUpdated();
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
  checkConfig();
  
  Serial.printf("setup heap size: %u\n", ESP.getFreeHeap());
  arducamInit();
  configManager();
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
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
   //mqttReconnect();
  }
  #if WEB_SERVER == 1
    server.handleClient();
  #endif
  checkButton();

  if (!mqttClient.connected()) {
    ++mqttFailCount;
    ticker.attach(0.3, tick);
    checkButton();
    long now = millis();
    if (now - lastMqttReconnectAttempt > 5000) {
        lastMqttReconnectAttempt = now;
        if (mqttFailCount == 5) {
       //   mqttReconnect() = false;
          Serial.println(F("Connexion MQTT infructueuse après 5 essais --> mode config"));
          lastMqttReconnectAttempt = 0;
          mqttFailCount = 0;
          configManager();
        }
        if (mqttConnect()) {
            #if DEBUG == 1
              Serial.print(F("Attempting MQTT connection to : "));
              Serial.println(mqttServer);
            #endif
            lastMqttReconnectAttempt = 0;
        }
    }
  }else {
      wifiFailCount = 0;
      if (STATE_LED == LOW); digitalWrite(STATE_LED, HIGH);
      ticker.detach();
      if (timeStatus() != timeNotSet) {
        if (now() != prevDisplay) { //update the display only if time has changed
          prevDisplay = now();
          digitalClockDisplay();
        }
      }
      if (transmitNow) { // checks if the buffer is full
      }
      //Capture();
      mqttClient.loop();
  }
    
 if ( WiFi.status() != WL_CONNECTED) { // WiFiMulti.run() != WL_CONNECTED
    ticker.attach(0.1, tick);
    ++wifiFailCount;
    if (wifiFailCount == 15) {
      ticker.detach();
      configManager();
    }  
  //checkButton();
  }
}
