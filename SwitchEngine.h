#ifndef SwitchEngine_h
  #define SwitchEngine_h
  #include "Arduino.h"
  #include "lifx.h"
  #include "SwitchTypen.h"
  #include "IInputDevice.h"
  #include "MotionDetector.h"
  #include "MultiButton.h"
  #include "MqttService.h"
  #include <WiFiUdp.h>
  #include <ESP8266WiFi.h>
  
  class SwitchEngine {
    public:
      SwitchEngine(const uint8_t *buttonPins, uint8_t buttonPinsSize, MqttService* mqttService);
      LifxDevice &elementAt(uint8_t index);
      LifxDevice* &devices();
      void tick();
      uint8_t countFoundDevices();
      void setBroadcastIp(IPAddress broadcastIp);    
      void setPinDevices();
      const uint8_t *buttonPins;      
      uint8_t buttonPinsSize;    
      void searchDevices();
      //const uint8_t *pinDevices; // 0 = button | 1 = sensor   
      PinConfig* pinConfig;
      
      void mqttCallback(char* topic, byte* payload, unsigned int length);
    private:            
      bool needsRefresh;
      unsigned long refreshRequestTime;
       int _lifxDevicesFoundCounter = 0;
      WiFiUDP _udp;
      IPAddress _broadcastIp; // =  IPAddress(255,255,255,255); //IPAddress(10,0,0,19);10.0.0.19
      byte queuedActions[8]; //per switch 
      LifxDevice *_lifxDevices; // = LifxDevice[10]; //currently 10 devices max
      
      IInputDevice **buttons;      
      
      MqttService* mqttService;
      MqttState* mqttState;  //todo => ButtonState
      void executeMqttAction(byte pinIdx, bool setValue);
      
      
      //void updateDevices(IPAddress** adresses, byte adressesSize);
      void updateDevice(IPAddress* address);
      //void togglePower(int pinButton);      
      void setPower(byte pinIdx, LifxDevice** devices, byte devicesSize, bool powerState );
      void togglePower(byte pinIdx, LifxDevice** devices, byte devicesSize);
      bool executeAction(byte pinIdx); //void executeAction();
      byte countDevicesOnSwitch(byte pinIdx);
      bool devicesOnSwitch(byte pinIdx, LifxDevice** devices, byte devicesSize);
      void executeMqttAction(byte pinIdx, const char* value);
      bool readUdpPacket();
      
      bool _macEquals(byte *array1, byte *array2);     
      void _printLifxPacket(LifxPacket &request, byte payload[], int payloadsize);       
  };
#endif
