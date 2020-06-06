#ifndef SwitchTypen_h
  #define SwitchTypen_h
  #include "Arduino.h"
  #include "lifx.h"
  #include <ESP8266WiFi.h>

union SwitchGroup {
  uint8_t raw[]; //1byte
  struct {
    uint8_t switch0:1;
    uint8_t switch1:1;
    uint8_t switch2:1;
    uint8_t switch3:1;
    uint8_t switch4:1;
    uint8_t switch5:1;
    uint8_t switch6:1;
    uint8_t switch7:1;    
    //uint8_t switch8:1;    
  }; 
};

typedef struct LifxDevice {
  IPAddress ip;   //4 bytes
  uint8_t mac[6];    //+6  = 10
  char name[32];  //+32 = 42
  //bool selected;  //+1 = 43
  SwitchGroup groups; //1 byte = 43  //rename to buttons?
  HSBK colorState; //8 byte = 51
  bool powerState; //1byte = 52
  unsigned long lastUpdateTime; //8bytes. = 60
  unsigned long updateRequestedTime;
}; // = ^^ bytes / device

//typedef struct QueuedAction {
//  byte switchIdx;
//  int action; // 0 = none; 1 PowerToggle; 2 Fade; 3???
//};

typedef struct MqttState {
  boolean powerState;
  uint16_t brightnessState;
};

typedef struct PinConfig {
  uint8_t type; //0 = button | 1 = sensor   (1 byte = tot 255 mogelijkheden!)
  uint8_t mqttEnabled; //( 1byte!)
  char name[33]; //32bytes + 1 stop byte
};

const uint8_t ACTION_NONE = 0;
const uint8_t ACTION_TOGGLE = 1;
const uint8_t ACTION_ON_FULL_BRI = 2;
const uint8_t ACTION_FADE_BRI = 3;
const uint8_t ACTION_RESERVED = 4; //long hold
const uint8_t ACTION_OFF = 5;
const uint8_t ACTION_ON = 6;
#endif
