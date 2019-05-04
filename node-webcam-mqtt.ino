#include <FS.h>
#include <advancedSerial.h>
#include <ArduinoJson.h>
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
#if CLIENT_SECURE == 1
#include <WiFiClientSecure.h>
#endif
#include <PubSubClient.h>
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
#if CLIENT_SECURE == 0
WiFiClient wifiClient;
#elif CLIENT_SECURE == 1
WiFiClientSecure wifiClient;
#endif
#if WEB_SERVER == 1
ESP8266WebServer server(80);
#endif
PubSubClient mqttClient(wifiClient);
Bounce debouncer = Bounce();
Ticker ticker;
Config config;
Message message;
WiFiManager wifiManager;

ArduCAM myCAM(OV2640, CS);

// UTILS
void checkSerial();
void loadConfig(const String filename, Config &config);
void saveConfig(const String filename, Config &config);
void initDefaultConfig(const String filename, Config &config);
void connectWifi();
void tick();
void setPins();
void checkButton();
void setReboot();
void setDefault();
void getDeviceId(Config &config);
void updateFile(const String fileName, int value);
void getUpdated(int which, const char* url, const char* fingerprint);
void setPinsRebootUart();

// SETTINGS
WiFiManagerParameter customMqttServer("server", "mqtt server", config.mqttServer, sizeof(config.mqttServer));
WiFiManagerParameter customMqttPort("port", "mqtt port", config.mqttPort, sizeof(config.mqttPort));
WiFiManagerParameter customMqttUser("user", "mqtt user", config.mqttUser, sizeof(config.mqttUser));
WiFiManagerParameter customMqttPassword("password", "mqtt password", config.mqttPassword, sizeof(config.mqttPassword));
WiFiManagerParameter customCamResolution("resolution", "cam resolution", config.camResolution, sizeof(config.camResolution));
WiFiManagerParameter customCamFpm("fpm", "cam fpm", config.camFpm, sizeof(config.camFpm));
void saveConfigCallback();
void quitConfigMode();
void configModeCallback (WiFiManager *myWiFiManager);
void initConfigManager(Config &config);
void configManager(Config &config);

// MQTT
#if MQTT_CLIENT == 1
void generateMqttClientId(Config &config);
void mqttInit(Config &config);
void mqttError();
void mqttReconnect(Config &config);
void mqttCallback(char* topic, byte* payload, unsigned int length);
#endif

// ALOES
void setSensors(Config &config);
void setSensorRoutes(Config &config, const char* objectId, const char* sensorId, const char* resourceId, size_t index);
void presentSensors(Config &config);
void setMessage(Message &message, char method[5], char* objectId, char* sensorId, char* resourceId, char* payload );
void sendMessage(Config &config, Message &message );
void parseMessage(Message &message);
void parseTopic(Message &message, char* topic);

// HTTP
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

// CAPTURE
void arducamInit();
void setCamResolution(int reso);
void setFPM(int interv);
void start_capture();
void camCapture(ArduCAM myCAM);
void serverCapture(ArduCAM myCAM);
void serverStream(ArduCAM myCAM);

void before() {
  Serial.begin(BAUD_RATE);
  //checkSerial();
#if DEBUG != 0
  aSerial.setPrinter(Serial);
#elif DEBUG == 0
  aSerial.off();
  Serial.setDebugOutput(false);
#endif

#if DEBUG == 1
  aSerial.setFilter(Level::v);
#elif DEBUG == 2
  aSerial.setFilter(Level::vv);
#elif DEBUG == 3
  aSerial.setFilter(Level::vvv);
#elif DEBUG == 4
  aSerial.setFilter(Level::vvvv);
#endif
  aSerial.v().p(F("====== ")).p(SKETCH_NAME).pln(F(" ======"));
  aSerial.v().println(F("====== Before setup ======"));
  for (uint8_t t = 4; t > 0; t--) {
    aSerial.vvv().print(F("[SETUP] WAIT ")).print(t).println(" ...");
    Serial.flush();
    delay(1000);
  }

  if (wifiResetConfig) { // add a button ?
    WiFiManager wifiManager;
    wifiManager.resetSettings();
  }
  if (resetConfig) {
    setDefault();
  }
  randomSeed(micros());
  getDeviceId(config);
  aSerial.vvv().print(F("before heap size : ")).println( ESP.getFreeHeap());
}

void setup() {
  before();
  aSerial.v().println(F("====== Setup started ======"));
  aSerial.vvv().pln(F("mounting FS..."));
  if (SPIFFS.begin()) {
    aSerial.vvv().pln(F("FS mounted"));
    if (SPIFFS.exists(configFileName)) {
      loadConfig(configFileName, config);
    } else {
      generateMqttClientId(config);
      initDefaultConfig(configFileName, config);
    }
  } else {
    aSerial.vv().pln(F("Failed to mount FS."));
  }
  //  if (config.otaSignal == 1 ) {
  //    //WiFi.persistent(false);
  //    String ssid = WiFi.SSID();
  //    String pass = WiFi.psk();
  //    WiFiMulti.addAP(ssid.c_str(), pass.c_str());
  //    while (WiFiMulti.run() != WL_CONNECTED) { //use this when using ESP8266WiFiMulti.h
  //      aSerial.vvv().println(F("Attempting Wifi connection.... "));
  //      delay(500);
  //    }
  //    aSerial.vv().print(F("Wifi connected. IP Address : ")).println(WiFi.localIP());
  //    //getUpdated(int which, const char* url, const char* fingerprint);
  //  }

  setPins();
  setSensors(config);

  ticker.attach(1.5, tick);
  connectWifi();
#if MQTT_CLIENT == 1
  mqttInit(config);
#endif
  initConfigManager(config);
#if NTP_SERVER == 1
  aSerial.vvv().println(F("Starting UDP"));
  Udp.begin(localPort);
  aSerial.vvv().print(F("Local port : ")).println(Udp.localPort());
  aSerial.vvvv().println(F("Waiting for sync "));
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  delay(100);
#endif
#if WEB_SERVER == 1
  server.on("/capture", HTTP_GET, serverCapture);
  server.on("/stream", HTTP_GET, serverStream);
  server.onNotFound(handleNotFound);
  server.begin();
  aSerial.vvv().println(F("Web server started"));
#endif
  aSerial.vvv().print(F("setup heap size : ")).println(ESP.getFreeHeap());
  ticker.detach();
  digitalWrite(STATE_LED, HIGH);

  arducamInit();

  aSerial.v().println(F("====== Setup ended ======"));
}

void loop() {
  if ( !executeOnce ) {
    executeOnce = true;
    aSerial.v().println(F("====== Loop started ======"));
  }

#if MQTT_CLIENT == 1
  if (!mqttClient.connected()) {
    mqttReconnect(config);
  }
  ticker.detach();
  mqttClient.loop();
#endif
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

  boolean changed = debouncer.update();
  if ( changed ) {
    checkButton();
  }
  if  ( buttonState == 1 ) {
    if ( millis() - buttonPressTimeStamp >= debouncerInterval ) {
      buttonPressTimeStamp = millis();
      aSerial.vvv().pln(F("Retriggering button"));
      if ( manualConfig == true) {
        manualConfig = false;
      }
      else {
        manualConfig = true;
        configManager(config);
      }
    }
  }
}
