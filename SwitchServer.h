#ifndef SwitchServer_h
  #define SwitchServer_h
  #include "Arduino.h"
  #include "SwitchTypen.h" 
  #include <ESP8266WiFi.h>
  //#include <ESP8266WebServer.h>
  #include <ESP8266WebServerSecure.h>
  #include "SwitchEngine.h"
  class SwitchServer {
    public:
      SwitchServer(BearSSL::ESP8266WebServerSecure *server, SwitchEngine* switchEngine); //LifxDevice** lifxDevices); //, LifxDevice _lifxDevices[], int arrayMaxBoundary);
      void begin();
      void handleClient();
      void handleRoot();
      void handleNotFound();
      void handleLifxDiscovery();      
      void handleFavicon();
    private:
      LifxDevice *lifxDevices;
      int lifxDevicesCount;
      BearSSL::ESP8266WebServerSecure *_server;
      String htmlEntities(String str);
      SwitchEngine* _switchEngine;
      void getRoot();
      void postRoot();
      String webTemplateStart; // = "<!DOCTYPE html><html><head><title>HA Switch</title></head><body>";
      String webTemplateEnd; // = "</body></html>";
      void sendInput(const char* type, String name, String value, bool checkboxChecked);
      void sendInputBox(String name, String value);
      void sendCheckbox(String name, String value, bool checked);      
      void sendDeviceSelectBox(const byte pinIdx, uint8_t value);
     
  };
#endif 
