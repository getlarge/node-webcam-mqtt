//// SKETCH
#define SKETCH_NAME "node-webcam-mqtt"
#define SKETCH_VERSION "0.3"

/// GLOBALS
#define DEBUG 1
#define DEBUG_WM 1
#define DEBUG_PAYLOAD 0
#define BAUD_RATE 115200

bool resetConfig = false, wifiResetConfig = false; // set to true to reset FS and/or Wifimanager, don't forget to set this to false after
unsigned long configTimeout1 = 300, configTimeout2 = 180, lastMqttReconnectAttempt = 0, lastWifiReconnectAttempt = 0;
bool shouldSaveConfig = true, executeOnce = false, manualConfig = false;
int configCount = 0, wifiFailCount = 0, mqttFailCount = 0, configMode = 0;
bool transmitNow = false;

// ESP
#define OTA_BUTTON_PIN D0
#define STATE_LED D4
#define CS 16 //// ARDUCAM

/// NETWORK PARAMS
#define CLIENT_SECURE 0
#define MQTT_CLIENT 1
#define WEB_SERVER 0
#define WEB_SERVER_SECURE 0
#define NTP_SERVER 0

char devicePass[30]="motdepasse", deviceId[20], devicePrefix[8] = "Camera";

struct mqttConfig {
  char mqtt_server[40];
  char mqtt_port[6];
  char mqtt_user[20];
  char mqtt_password[30];
  char mqtt_topic_out[50];
  char mqtt_topic_in[50];
};

char out[20]= "/camera/send", out1[20]= "/camera/eof";
char in[10]= "/in/#";
//char in[10]= "/in";

//const char *httpServer; int httpPort;
//const char *postDestination;
//const char* otaUrl = "https://app.aloes.io/firmware/NodeWebcam.ino.bin";
//const char* currentVersion = "4712";
//const char* httpsFingerprint = "1D AE 00 E7 68 70 87 09 A6 1D 27 76 F5 85 C0 F3 AB F2 60 9F"; 

static const size_t bufferSize = 2048; // 4096; //2048; //1024;

//// FILE MANAGER
static const int fileSpaceOffset = 700000;
const String otaFile = "ota.txt"; const String resFile = "res.txt"; const String fpmFile = "fpm.txt";
int fileTotalKB = 0;
int fileUsedKB = 0; 
int fileCount = 0;
String errMsg = "";
int resolution = 4; int fpm = 0; 
int otaSignal = 0;
int minDelayBetweenframes, interval1 = 1000; 


//// NTP
static const char ntpServerName[] = "fr.pool.ntp.org";
const int timeZone = 1;     // Central European Time
unsigned int localPort = 8888;  // local port to listen for UDP packets
