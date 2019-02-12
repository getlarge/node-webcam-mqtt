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
ArduCAM myCAM(OV2640, CS);
Bounce debouncer = Bounce();
Ticker ticker;
Config config;
Message message;
WiFiManager wifiManager;

// UTILS
void checkSerial();
void loadConfig(const String filename, Config &config);
void saveConfig(const String filename, Config &config);
void quitConfigMode();
void tick();
void setPins();
void checkButton();
void setReboot();
void setDefault();
void getDeviceId(Config &config);
void checkFile(const String fileName, int value);
void updateFile(const String fileName, int value);
void getUpdated(int which, const char* url, const char* fingerprint);
void setPinsRebootUart();
void connectWifi();
void loadRoutes(Message &message, Config &config);

// SETTINGS
void saveConfigCallback();
void configModeCallback (WiFiManager *myWiFiManager);
void configManager(Config &config);

// CAPTURE
void arducamInit();
void setCamResolution(int reso);
void setFPM(int interv);
void start_capture();
void camCapture(ArduCAM myCAM, Message &message);
void serverCapture(ArduCAM myCAM,  Message &message);
void serverStream(ArduCAM myCAM,  Message &message);

// MQTT
void mqttInit(Config &config);
void presentSensors(const char* objectId, const char* sensorId, const char* resourceId, const char* payload);
void mqttError();
boolean mqttConnect(Config &config);
void mqttCallback(char* topic, byte* payload, unsigned int length);
void parseMessage(Message &message);

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
  checkFile(otaFile, otaSignal);
  if (otaSignal == 1 ) {
    //WiFi.persistent(false);
    String ssid = WiFi.SSID();
    String pass = WiFi.psk();
    WiFiMulti.addAP(ssid.c_str(), pass.c_str());
    while (WiFiMulti.run() != WL_CONNECTED) { //use this when using ESP8266WiFiMulti.h
      aSerial.vvv().println(F("Attempting Wifi connection.... "));
      delay(500);
    }
    aSerial.vv().print(F("Wifi connected. IP Address : ")).println(WiFi.localIP());
    //getUpdated(int which, const char* url, const char* fingerprint);
  }
  for (uint8_t t = 4; t > 0; t--) {
    aSerial.vvv().print(F("[SETUP] WAIT ")).print(t).println(" ...");
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
  aSerial.vvv().print(F("before heap size : ")).println( ESP.getFreeHeap());
}

void setup() {
  before();
  aSerial.v().println(F("====== Setup started ======"));
  randomSeed(micros());
  getDeviceId(config);
  loadConfig(configFileName, config);
  loadRoutes(message, config);
  configManager(config);
  arducamInit();
  delay(1000);
#if WEB_SERVER == 0
  mqttInit(config);
#elif WEB_SERVER == 1
  connectWifi();
#endif
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
  aSerial.v().println(F("====== Setup ended ======"));
}

void loop() {
  if ( ! executeOnce) {
    executeOnce = true;
    delay(500);
    aSerial.v().println(F("====== Loop started ======"));
  }
  //
#if WEB_SERVER == 0
  if (!mqttClient.connected()) {
    long now = millis();
    ticker.attach(0.3, tick);
    if (now - lastMqttReconnectAttempt > reconnectInterval) {
      lastMqttReconnectAttempt = now;
      ++mqttFailCount;
      aSerial.vv().p(F("MQTT connection failure #")).p(mqttFailCount).pln(F(" --> reconnect"));
      if (mqttConnect(config)) {
        mqttFailCount = 0;
        lastMqttReconnectAttempt = 0;
      }
      else if (mqttFailCount > mqttMaxFailedCount) {
        aSerial.vv().p(mqttMaxFailedCount).pln(F("+ MQTT connection failure --> config mode"));
        //  lastMqttReconnectAttempt = 0;
        //  mqttFailCount = 0;
        configManager(config);
      }
    }
  } else {
    ticker.detach();
    mqttClient.loop();
  }
#elif WEB_SERVER == 1
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
