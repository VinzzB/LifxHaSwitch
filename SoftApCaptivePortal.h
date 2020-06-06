#ifndef SoftApCaptivePortal_h
  #define SoftApCaptivePortal_h
  #include "Arduino.h"
  #include <DNSServer.h>
  #include <ESP8266WiFi.h>  
  #include <WiFiClient.h>
  #include <ESP8266WebServer.h>
 // #include <ESP8266WebServerSecure.h>
  
  typedef struct WlanConfig {
    char ssid[32] = "";
    char password[32] = "";
  };

  
  class SoftApCaptivePortal {
    public:
      void setup(ESP8266WebServer *server, const char *softAP_ssid, const char *softAP_password, const char *myHostname, IPAddress apIP, IPAddress netMsk, int(*connectToWifi)(void));
      void tick();
     void handleWifiSave();
     void handleWifi();
     boolean redirectToPortal();
     boolean wlanConnected;
     boolean requestedWlanConnect();
     WlanConfig wlanConfig;
    private:
    /* PROPS */
      int(*connectToWifi)(void);
      ESP8266WebServer *server;
      DNSServer dnsServer; 
      IPAddress apIP;
      const char *myHostname;
      const char *softAP_ssid;
    /* METHODS */
      String htmlEntities(String str);
      boolean isIp(String str);
      String toStringIp(IPAddress ip);
      unsigned long requestedApStopOn;
      unsigned long requestedWlanConnectOn;
      void requestApStop();
      void requestWlanConnect();
  };
#endif
