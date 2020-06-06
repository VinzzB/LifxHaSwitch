/*
HelloServerSecure
  Udp NTP Client

*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include "SwitchServer.h"
#include "MqttService.h"
#include "SoftApCaptivePortal.h"

//Todo: bewerkbara maken en opslaan in EEPROM
#ifndef STASSID
#define STASSID "WIFI"
#define STAPSK  "een wifi wachtwoord"
#endif

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


static const char serverCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFYjCCBEqgAwIBAgISA73nYiYX+K5PeB4b5uT9ukVcMA0GCSqGSIb3DQEBCwUA
MEoxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBFbmNyeXB0MSMwIQYDVQQD
ExpMZXQncyBFbmNyeXB0IEF1dGhvcml0eSBYMzAeFw0yMDA0MTIxNDU1NDBaFw0y
MDA3MTExNDU1NDBaMBcxFTATBgNVBAMTDGxhbi52aW56ei5iZTCCASIwDQYJKoZI
hvcNAQEBBQADggEPADCCAQoCggEBAMIDsGDmnLrWRs9NGV24N5xNjNAtsV9kWKZU
di4clLh3dgzx0fug9sIGWQgRaQOhNf+cKsVzI+gNzfd9U39LEYcu/eb62OaUzfwU
H07WAJiQsuWQdphTwaWt79+BUSb16+QLTPNbsMpKMLcqW9MD5ZCPeWL/46hvmKHl
LPkn2L/9i1OHwSfVDq79k7CRNt8RGIL+VobFAd5PgZ4CSvfesDXWRoRnrbRQ5UOM
FZB5RWPC8a768SPghu/dB6YV6bZBUilL+ghP2QnLwW007VltJBpaHOcyGJZqoU9l
TFY6ZD4nqxqVATgODK/h6VKEeroZEERFZuxkxRLhPZyO7F9V41sCAwEAAaOCAnMw
ggJvMA4GA1UdDwEB/wQEAwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUH
AwIwDAYDVR0TAQH/BAIwADAdBgNVHQ4EFgQUa4FvEfnkLUwylfIfIIVRe1TSHQAw
HwYDVR0jBBgwFoAUqEpqYwR93brm0Tm3pkVl7/Oo7KEwbwYIKwYBBQUHAQEEYzBh
MC4GCCsGAQUFBzABhiJodHRwOi8vb2NzcC5pbnQteDMubGV0c2VuY3J5cHQub3Jn
MC8GCCsGAQUFBzAChiNodHRwOi8vY2VydC5pbnQteDMubGV0c2VuY3J5cHQub3Jn
LzAnBgNVHREEIDAegg4qLmxhbi52aW56ei5iZYIMbGFuLnZpbnp6LmJlMEwGA1Ud
IARFMEMwCAYGZ4EMAQIBMDcGCysGAQQBgt8TAQEBMCgwJgYIKwYBBQUHAgEWGmh0
dHA6Ly9jcHMubGV0c2VuY3J5cHQub3JnMIIBBgYKKwYBBAHWeQIEAgSB9wSB9ADy
AHcA5xLysDd+GmL7jskMYYTx6ns3y1YdESZb8+DzS/JBVG4AAAFxbxqCaAAABAMA
SDBGAiEA+5JONRZYxnC1YiXyL1sy5/lBXTjqK+z/1spxV0gsFOYCIQCsuSEb0TDv
Q+OpEcVRgaKB/8qohNJqGenC6ymvjW43SAB3AAe3XBvlfWj/8bDGHSMVx7rmV3xX
lLdq7rxhOhpp06IcAAABcW8agpEAAAQDAEgwRgIhANWLk/w8zoUvubJqg9jx7qhW
vBNEyT92cZQRrJzYq6h+AiEA+FLiTyQPRzueC/gpLQCp/34YF2oz6IC3lV+25s7i
7YUwDQYJKoZIhvcNAQELBQADggEBAEDvCdgtzZusaU0MkYOYkrn8d7OwxGvf1v7a
66gwB8ha/fmR1Plbe6dkB7Oz4QGHmrVs0ehJvCNxRHPGlWnt/RiE01JY/lxO0eA2
sA4+9AkF/6gN2Nr0MrUJn5Osq4XrX94B83adOeC6ZIfcwqOmH2pC/LLRPPTdPrwO
3mvxzos/tY+YVBitR8Z+ZpqwTS8FIggvyYmgAvcgnFZ1PvkbJCf93FGD/LRttbtm
hrscglsir71Wk2+q39fsEoHs7L8uncHGHqjcjzJRcvwIMzl8FlyK0aTd1pT06laJ
cFsHwCEr6a5wn0ySTNhw3/SfSJ5ht58HxcR3BIrfsqjwE1qIqFE=
-----END CERTIFICATE-----

-----BEGIN CERTIFICATE-----
MIIFjTCCA3WgAwIBAgIRANOxciY0IzLc9AUoUSrsnGowDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTYxMDA2MTU0MzU1
WhcNMjExMDA2MTU0MzU1WjBKMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg
RW5jcnlwdDEjMCEGA1UEAxMaTGV0J3MgRW5jcnlwdCBBdXRob3JpdHkgWDMwggEi
MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCc0wzwWuUuR7dyXTeDs2hjMOrX
NSYZJeG9vjXxcJIvt7hLQQWrqZ41CFjssSrEaIcLo+N15Obzp2JxunmBYB/XkZqf
89B4Z3HIaQ6Vkc/+5pnpYDxIzH7KTXcSJJ1HG1rrueweNwAcnKx7pwXqzkrrvUHl
Npi5y/1tPJZo3yMqQpAMhnRnyH+lmrhSYRQTP2XpgofL2/oOVvaGifOFP5eGr7Dc
Gu9rDZUWfcQroGWymQQ2dYBrrErzG5BJeC+ilk8qICUpBMZ0wNAxzY8xOJUWuqgz
uEPxsR/DMH+ieTETPS02+OP88jNquTkxxa/EjQ0dZBYzqvqEKbbUC8DYfcOTAgMB
AAGjggFnMIIBYzAOBgNVHQ8BAf8EBAMCAYYwEgYDVR0TAQH/BAgwBgEB/wIBADBU
BgNVHSAETTBLMAgGBmeBDAECATA/BgsrBgEEAYLfEwEBATAwMC4GCCsGAQUFBwIB
FiJodHRwOi8vY3BzLnJvb3QteDEubGV0c2VuY3J5cHQub3JnMB0GA1UdDgQWBBSo
SmpjBH3duubRObemRWXv86jsoTAzBgNVHR8ELDAqMCigJqAkhiJodHRwOi8vY3Js
LnJvb3QteDEubGV0c2VuY3J5cHQub3JnMHIGCCsGAQUFBwEBBGYwZDAwBggrBgEF
BQcwAYYkaHR0cDovL29jc3Aucm9vdC14MS5sZXRzZW5jcnlwdC5vcmcvMDAGCCsG
AQUFBzAChiRodHRwOi8vY2VydC5yb290LXgxLmxldHNlbmNyeXB0Lm9yZy8wHwYD
VR0jBBgwFoAUebRZ5nu25eQBc4AIiMgaWPbpm24wDQYJKoZIhvcNAQELBQADggIB
ABnPdSA0LTqmRf/Q1eaM2jLonG4bQdEnqOJQ8nCqxOeTRrToEKtwT++36gTSlBGx
A/5dut82jJQ2jxN8RI8L9QFXrWi4xXnA2EqA10yjHiR6H9cj6MFiOnb5In1eWsRM
UM2v3e9tNsCAgBukPHAg1lQh07rvFKm/Bz9BCjaxorALINUfZ9DD64j2igLIxle2
DPxW8dI/F2loHMjXZjqG8RkqZUdoxtID5+90FgsGIfkMpqgRS05f4zPbCEHqCXl1
eO5HyELTgcVlLXXQDgAWnRzut1hFJeczY1tjQQno6f6s+nMydLN26WuU4s3UYvOu
OsUxRlJu7TSRHqDC3lSE5XggVkzdaPkuKGQbGpny+01/47hfXXNB7HntWNZ6N2Vw
p7G6OfY+YQrZwIaQmhrIqJZuigsrbe3W+gdn5ykE9+Ky0VgVUsfxo52mwFYs1JKY
2PGDuWx8M6DlS6qQkvHaRUo0FMd8TsSlbF0/v965qGFKhSDeQoMpYnwcmQilRh/0
ayLThlHLN81gSkJjVrPI0Y8xCVPB4twb1PFUd2fPM3sA1tJ83sZ5v8vgFv2yofKR
PB0t6JzUA81mSqM3kxl5e+IZwhYAyO0OTg3/fs8HqGTNKd9BqoUwSRBzp06JMg5b
rUCGwbCUDI0mxadJ3Bz4WxR6fyNpBK2yAinWEsikxqEt
-----END CERTIFICATE-----

-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

static const char serverKey[] PROGMEM =  R"EOF(
-----BEGIN RSA PRIVATE KEY-----
....
-----END RSA PRIVATE KEY-----
)EOF";
        
//Setup services (MQTT protocol / switchEngine / WebServer)
WiFiClient espClient;
PubSubClient pubSubClient(espClient);
ESP8266WebServer httpServer(80); //ESP8266WebServer = Http server and uses WiFiServer. WiFiServer = TCP server.
BearSSL::ESP8266WebServerSecure server(443); 
MqttService mqttService(&pubSubClient);
SwitchEngine switchEngine(buttonPins, sizeof(buttonPins), &mqttService);
SwitchServer webServer(&server, &switchEngine);

/* Soft AP network parameters */
IPAddress apIP(172, 217, 28, 1);
IPAddress apNetMask(255, 255, 255, 0);
const char *softAP_ssid =  "HA-Switch";
const char *softAP_password = "12345678";
const char *myHostname = "haswitch";
// void setup(ESP8266WebServer *server, const char *softAP_ssid, const char *softAP_password, const char *myHostname, IPAddress apIP, IPAddress netMsk);
SoftApCaptivePortal captivePortal;

//void printSwitchNames() {
//  for(int x = 0; x < 4; x++) {
//    Serial.print("Switch ")  ;
//    Serial.print(x+1);
//    Serial.print(" - Name: ");
//    Serial.println(switchEngine.pinConfig[x].name);
//  }
//}

void setup() {    
  softAP_ssid = "HA-Switch 2";
  WiFi.persistent(false);
  //myHostname = WiFi.hostname().c_str();
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
 // printSwitchNames();
  // set initial LED state (off = HIGH)
  digitalWrite(ledPin, HIGH);
  captivePortal.setup(&server,softAP_ssid,softAP_password , myHostname,apIP,apNetMask, connectToWiFiNetwork);
  Serial.println("AP initialized!");

  httpServer.onNotFound([]() {
    Serial.print("Redirect HTTP request: ");
    Serial.println(httpServer.uri());
    httpServer.sendHeader("Location", String("https://") + myHostname + ".lan.vinzz.be", true); // + toStringIp(server->client().localIP()), true);
    httpServer.send(302, "text/plain", "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
  });
  httpServer.begin();
  server.getServer().setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));
  Serial.print(" - Hostname: ");
  Serial.println(myHostname);  
//  printSwitchNames();
  
  //Establish a WiFi connection
 // connectToWiFiNetwork();

  
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
      captivePortal.handleWifi();
    else
      webServer.handleRoot(); 
     
  });
  server.on("/discover",[]()  { if(captivePortal.redirectToPortal()) return; webServer.handleLifxDiscovery(); });
  server.onNotFound([]()        { if(captivePortal.redirectToPortal()) return; webServer.handleNotFound(); });
  //captive portal setup
  //server.on("/wifi", []()       { captivePortal.handleWifi(); });
  server.on("/wifisave",HTTP_POST,[]()    { captivePortal.handleWifiSave(); });
  server.on("/reset",[]()    { 
    Serial.println("FACTORY DEFAULTS!!!");
    server.sendHeader("Location", "/", true);
    server.send ( 302, "text/plain", "");
 });
  server.on("/generate_204",[](){ if(captivePortal.redirectToPortal()) return; });  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink",[]()      { if(captivePortal.redirectToPortal()) return; });  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/favicon.ico", []() {
    char ico[sizeof(favicon)];
    memcpy_P(ico, favicon, sizeof(favicon));
    server.send(200, "image/x-icon", ico, sizeof(ico));
  });
  server.begin();
  Serial.println("HTTP server started");
 // printSwitchNames();
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
  httpServer.handleClient();
  server.handleClient();  
}

int connectToWiFiNetwork() {  
//  if(!captivePortal.requestWlanConnect)  
//    return WL_DISCONNECTED;  
  Serial.print("Connecting to WiFi network...");
//  printSwitchNames();
  WiFi.disconnect();
  //WiFi.mode(WIFI_STA);
  WiFi.hostname(myHostname);  
  WiFi.begin(captivePortal.wlanConfig.ssid, captivePortal.wlanConfig.password);
  int wifiStatus = WiFi.waitForConnectResult(20000);
//  while (wifiStatus = WiFi.status() == WL_IDLE_STATUS) {
//    Serial.print('.');
//    delay(500);
//  }  
  if(wifiStatus == WL_CONNECTED) {
    IPAddress localBroadcastIp = IPAddress((uint32_t(WiFi.localIP()) & uint32_t(WiFi.subnetMask())) | ~uint32_t(WiFi.subnetMask()));
    Serial.print("\r\nConnected! IP address: ");
    Serial.print(WiFi.localIP());  
    Serial.print(" - Local broadcast address: ");
    Serial.println(localBroadcastIp);  
    switchEngine.setBroadcastIp(localBroadcastIp);      
    Serial.print(" - Hostname: ");
    Serial.println(myHostname);      
    
  } else {
    WiFi.disconnect();    
  }
 // printSwitchNames();
  return wifiStatus;
}
