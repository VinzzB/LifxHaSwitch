/*
HelloServerSecure
  Udp NTP Client

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include "SwitchServer.h"
#include "MqttService.h"
#include "SoftApCaptivePortal.h"
#include <FS.h>

//Todo: bewerkbara maken en opslaan in EEPROM
#ifndef STASSID
#define STASSID "WIFI"
#define STAPSK  "een wifi wachtwoord"
#endif


String login = "admin";
const String realm = "HA-Switch Administration";
String H1 = "";
String authentication_failed = "User authentication has failed.";

// the LED pin nr (d4 on wemos)
const int ledPin = LED_BUILTIN;      
//setup input pins (switches / sensors)
//currently up to 8 pins
const uint8_t buttonPins[] = { D1, D2, D5, D6 };

const uint8_t favicon[] PROGMEM = {
        0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x10, 0x10, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0xB0, 0x00, 
        0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 
        0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFE, 0x7F, 0x00, 0x00, 0xFC, 0x3F, 
        0x00, 0x00, 0xFC, 0x3F, 0x00, 0x00, 0xFE, 0x7F, 0x00, 0x00, 0xFB, 0xDF, 0x00, 0x00, 0xF8, 0x1F, 
        0x00, 0x00, 0xEC, 0x37, 0x00, 0x00, 0xE6, 0x67, 0x00, 0x00, 0xE3, 0xC7, 0x00, 0x00, 0xB0, 0x0D, 
        0x00, 0x00, 0x98, 0x19, 0x00, 0x00, 0xCF, 0xF3, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0xF0, 0x0F, 
        0x00, 0x00, 0xFC, 0x3F, 0x00, 0x00 };
//Setup services (MQTT protocol / switchEngine / WebServer)
WiFiClient espClient;
PubSubClient pubSubClient(espClient);
ESP8266WebServer server(80); //ESP8266WebServer = Http server and uses WiFiServer. WiFiServer = TCP server.
MqttService mqttService(&pubSubClient);
SwitchEngine switchEngine(buttonPins, sizeof(buttonPins), &mqttService);
SwitchServer webServer(&server, &switchEngine);
SoftApCaptivePortal captivePortal;
/* Soft AP network parameters */
IPAddress apIP(172, 217, 28, 1);
IPAddress apNetMask(255, 255, 255, 0);
const char *softAP_ssid =  "HA-Switch";
const char *softAP_password = "12345678";
const char *myHostname = "haswitch";

void setup() {    
  H1=ESP8266WebServer::credentialHash(login,realm,"admin");
  softAP_ssid = "HA-Switch 2";
  WiFi.persistent(false);
  //myHostname = WiFi.hostname().c_str();
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  // set initial LED state (off = HIGH)
  digitalWrite(ledPin, HIGH);
  captivePortal.setup(&server,softAP_ssid,softAP_password , myHostname,apIP,apNetMask, connectToWiFiNetwork);
  Serial.println("AP initialized!");
  Serial.print(" - Hostname: ");
  Serial.println(myHostname);  

  //define endpoints for webServer.  
  server.on("/", []() { 
    
    WiFiClient client = server.client();
    if(client == NULL){
      //Client can be NULL when spawning a lot of requests by smashing F5 in the browser... 
      //Skipping these requests will prevent the mcu from crashing. The crash occurs when trying to read localIP() from non existent client.
      Serial.println("CLIENT IN REQUEST IS NULL! Skipping request.");       
      return;
    }
        
    if(captivePortal.redirectToPortal()) 
      return;       
      
    if (client.localIP() == apIP)
      captivePortal.handleWifi(true);
    else if (session_authenticated())
      webServer.handleRoot();     
  });
  server.on("/discover",[]()  { if(captivePortal.redirectToPortal() || !session_authenticated()) return; webServer.handleLifxDiscovery(); });  
  server.on("/wifi",HTTP_GET,[]() { 
    Serial.println("WiFi Get!");
    if(session_authenticated()) 
      captivePortal.handleWifi(false);
  });
  //captive portal setup
  server.on("/wifi",HTTP_POST,[]() { 
    Serial.print("WiFi Post! wlan Connected: ");
    Serial.print(captivePortal.wlanConnected);
    if(!captivePortal.wlanConnected || session_authenticated()) 
      captivePortal.handleWifiSave(); 
  });
  server.on("/reset",[]()    { 
    if(captivePortal.wlanConnected && !session_authenticated())
      return;
      
    Serial.println("FACTORY DEFAULTS!!!");
    server.sendHeader("Location", "/", true);
    server.send ( 302, "text/plain", "");
 });
//  server.on("/generate_204",[](){ if(captivePortal.redirectToPortal()) return; });  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
//  server.on("/fwlink",[]()      { if(captivePortal.redirectToPortal()) return; });  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/favicon.ico", []() {
    char ico[sizeof(favicon)];
    memcpy_P(ico, favicon, sizeof(favicon));
    server.send(200, "image/x-icon", ico, sizeof(ico));
  });
  server.onNotFound([]()        { if(captivePortal.redirectToPortal()) return; webServer.handleNotFound(); });
  server.begin();
  Serial.println("HTTP server started");
  //setup MQTT (PoC)
  pubSubClient.setServer("home.lan.vinzz.be",1883); //TODO!  (moet achter wifi connect komen + aanpasbaar adres)
  pubSubClient.setCallback([](char* topic, byte* payload, unsigned int length) {
    switchEngine.mqttCallback(topic,payload,length);
  });    
}

void loop() {      
  switchEngine.tick();
  captivePortal.tick();  
  if(captivePortal.wlanConnected)
    mqttService.tick();  
  server.handleClient();  
}

//This function checks whether the current session has been authenticated. If not, a request for credentials is sent.
bool session_authenticated() {
  Serial.println("Checking authentication.");
  if (server.authenticateDigest(login,H1)) {
    Serial.println("Authentication confirmed.");
    return true;
  } else  {
    Serial.println("Not authenticated. Requesting credentials.");
    server.requestAuthentication(DIGEST_AUTH,realm.c_str(),authentication_failed);
    // redirect();
    return false;
  }
}


int connectToWiFiNetwork() {  
  Serial.print("Connecting to WiFi network...");
  WiFi.disconnect();
  //WiFi.mode(WIFI_STA);
  WiFi.hostname(myHostname);  
  WiFi.begin(captivePortal.wlanConfig.ssid, captivePortal.wlanConfig.password);
  int wifiStatus = WiFi.waitForConnectResult(20000);
  if(wifiStatus == WL_CONNECTED) {
    IPAddress localBroadcastIp = IPAddress((uint32_t(WiFi.localIP()) & uint32_t(WiFi.subnetMask())) | ~uint32_t(WiFi.subnetMask()));
    Serial.print("\r\nConnected! IP address: ");
    Serial.print(WiFi.localIP());  
    Serial.print(" - Local broadcast address: ");
    Serial.println(localBroadcastIp);  
    switchEngine.setBroadcastIp(localBroadcastIp);      
    captivePortal.wlanConnected = true;
    Serial.print(" - Hostname: ");
    Serial.println(myHostname);      
    
  } else {
    WiFi.disconnect();    
  }
  return wifiStatus;
}
