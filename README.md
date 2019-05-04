# NODE WEBCAM MQTT

ESP8266 arduino sketch to interact with [Aloes backend](https://framagit.org/aloes/device-manager) MQTT API.

Adding camera functionnality with [Arducam](https://github.com/ArduCAM/Arduino) device and library.


## Requirements

Arduino IDE - download the latest from arduino

- https://www.arduino.cc/en/Main/Software

Packages for ESP8266

- Enter `http://arduino.esp8266.com/stable/package_esp8266com_index.json` into Additional Board Manager URLs field. 

following libraries are required :

- ArduinoJson
- Bounce2
- FS
- Ticker
- WifiManager

## Installation

```
git clone https://github/getlarge/node-webcam-mqtt.git
```

Edit Arduino/libraries/ArduCAM/memorysaver.h to :

```
#define OV2640_MINI_2MP
```

Edit Arduino/libraries/PubSubClient/src/PubSubClient.h to :

```
#define MQTT_MAX_PACKET_SIZE 4096
```


## Usage

- Open any .ino file of the folder with Arduino IDE

- Edit your preferences in `config.h`

- Upload the code on your ESP8266 board

#if using MQTT_CLIENT

- Copy the deviceId ( generated at setup on the serial interface ) to Aloes backend

- Enter device credentials in `config.h` or configure the board via the Access Point ( 192.168.244.1 )

#endif


## Reference

- resolutions:
- 0 = 160x120
- 1 = 176x144
- 2 = 320x240
- 3 = 352x288
- 4 = 640x480
- 5 = 800x600
- 6 = 1024x768
- 7 = 1280x1024
- 8 = 1600x1200


