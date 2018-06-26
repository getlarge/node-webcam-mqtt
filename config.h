//// SKETCH
#define SKETCH_NAME "node-webcam-mqtt"
#define SKETCH_VERSION "0.1"

/// GLOBALS
#define DEBUG 1
#define DEBUG_PAYLOAD 0
#define BAUD_RATE 115200
bool resetConfig = false, wifiResetConfig = false; // set to true to reset FS and/or Wifimanager, don't forget to set this to false after
unsigned long configTimeout1 = 300, configTimeout2 = 180, lastMqttReconnectAttempt = 0, lastWifiReconnectAttempt = 0;
unsigned long lastUpdate=0, lastRequest=0, lastPic = 0;
bool shouldSaveConfig = true, executeOnce = false, switchOnCam = false, otaSignalReceived = false, manualConfig = false;
int configCount = 0, wifiFailCount = 0, mqttFailCount = 0, configMode = 0, _otaSignal = 0;
bool transmitNow = false;

// ESP
#define OTA_BUTTON_PIN D3
#define STATE_LED D4
#define CS 16 //// ARDUCAM

/// NETWORK PARAMS
#define CLIENT_SECURE 0
#define WEB_SERVER 0
#define WEB_SERVER_SECURE 0

char devicePass[30]="motdepasse", deviceId[20], devicePrefix[8] = "Camera";
char mqtt_client_id[60], mqtt_user[40], mqtt_password[30], mqtt_server[40], mqtt_port[6], http_server[40], http_port[6]; 
char mqtt_topic_out[70], mqtt_topic_out2[70], mqtt_topic_in[70], out[10]= "/send", out2[10]= "/eof", in[10]= "/in/#";
char post_prefix[10] = "/cam/", post_destination[70]; 
int mqttPort, httpPort;
const char* mqttServer;
const char* mqttClientId;
const char* mqttUser;
const char* mqttPassword;
const char* mqttTopicOut;
const char* mqttTopicOut2;
const char* mqttTopicIn;
const char *postDestination;
const char *httpServer;
const char* otaUrl = "https://app.aloes.io/firmware/NodeWebcam.ino.bin";
const char* currentVersion = "4712";
const char* httpsFingerprint = "1D AE 00 E7 68 70 87 09 A6 1D 27 76 F5 85 C0 F3 AB F2 60 9F"; 

//static const size_t bufferSize = 1024; // 4096; //2048; //1024;
//static uint8_t buffer[bufferSize] = {0xFF};


//// FILE MANAGER
static const int fileSpaceOffset = 700000;
const String otaFile = "ota.txt";
const String fName = "res.txt";
const String f2Name = "fpm.txt";
int fileTotalKB = 0;
int fileUsedKB = 0; 
int fileCount = 0;
String errMsg = "";
int resolution = 4;
int minDelayBetweenframes, interval1 = 5000; 
int fpm = 0;

//// NTP
static const char ntpServerName[] = "fr.pool.ntp.org";
const int timeZone = 1;     // Central European Time
//const int timeZone = -5;  // Eastern Standard Time (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)
unsigned int localPort = 8888;  // local port to listen for UDP packets
