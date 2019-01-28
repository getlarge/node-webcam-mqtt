//// SKETCH
#define SKETCH_NAME "[ALOES NODE WEBCAM]"
#define SKETCH_VERSION "0.5"

/// GLOBALS
#define DEBUG 4 // from 0 to 4
#define BAUD_RATE 115200

bool resetConfig = false, wifiResetConfig = false; // set to true to reset FS and/or Wifimanager, don't forget to set this to false after
unsigned long configTimeout = 200, minDelayBetweenframes = 1000, reconnectInterval = 500, debouncerInterval = 2000;
unsigned long lastMqttReconnectAttempt = 0, lastWifiReconnectAttempt = 0, lastPictureAttempt = 0, buttonPressTimeStamp;
bool shouldSaveConfig = true, executeOnce = false, manualConfig = false;
bool transmitNow = false, transmitStream = false;
bool timelapse = false, base64encoding = false;
int configCount = 0, wifiFailCount = 0, mqttFailCount = 0, mqttMaxFailedCount = 10, configMode = 0, buttonState;
volatile byte state = LOW;

// ESP
#define BOUNCE_LOCK_OUT
#define OTA_BUTTON_PIN D3
#define STATE_LED D4

//// ARDUCAM
#define CS 16

/// NETWORK PARAMS
#define ID_TYPE 0 // 0 : prefix+espCHipId - 1 : prefix+MACadress - 2 : prefix+EUI-64 address
#define CLIENT_SECURE 0
#define MQTT_CLIENT 1
#define WEB_SERVER 0
#define NTP_SERVER 0

char static_ip[16] = "192.168.1.35";
char static_gw[16] = "192.168.1.254";
char static_sn[16] = "91.121.61.147";
char devicePass[40] = "motdepasse", devicePrefix[8] = "Camera";
char masterTopic[60], captureTopic[80], eofTopic[80], streamTopic[80];

/// Go deeper into message details, with commands and sensors table ?
//  "pattern": "+prefixedDevEui/+method/+ipsoObjectId/+sensorId/+ipsoResourcesId",
//struct messageFormat {
//  char *deviceId;
//  char in_prefix[10] = "/in";
//  char out_prefix[10] = "/out";
//  char sensor[25];
//  char command[25];
//  char* payload;
//};
struct messageFormat {
  char *deviceId;
  char in_prefix[10] = "-in";
  char out_prefix[10] = "-out";
  char method[5];
  char ipsoObjectId[5];
  char sensorId[25];
  char ipsoResourcesId[5];
  char* payload;
};

struct mqttConfig {
  char mqtt_server[40];
  char mqtt_port[6];
  char mqtt_client[40];
  char mqtt_user[30];
  char mqtt_password[70];
  char mqtt_topic_out[50];
  char mqtt_topic_in[50];
};

//// FILE / STREAM MANAGER
static const size_t camBufferSize = 1024; // 4096; //2048; //1024;
static const size_t objBufferSize = 256;
static const int fileSpaceOffset = 700000;
const String otaFile = "ota.txt"; const String resFile = "res.txt"; const String fpmFile = "fpm.txt"; const String configFileName = "config.json";
int fileTotalKB = 0;
int fileUsedKB = 0;
int fileCount = 0;
String errMsg = "";
int resolution = 4; int fpm = 3;
int otaSignal = 0;

//// NTP
#if NTP_SERVER == 1
static const char ntpServerName[] = "fr.pool.ntp.org";
const int timeZone = 1;     // Central European Time
unsigned int localPort = 8888;  // local port to listen for UDP packets
#endif
