
#include "Arduino.h"
#include "SoftApCaptivePortal.h"


void SoftApCaptivePortal::setup(BearSSL::ESP8266WebServerSecure *server, const char *softAP_ssid, const char *softAP_password, const char *myHostname, IPAddress apIP, IPAddress netMsk, int(*connectToWifi)(void))
{
  this->server = server;
  this->myHostname = myHostname;
  this->apIP = apIP;
  this->softAP_ssid = softAP_ssid;
  this->connectToWifi = connectToWifi;
  Serial.println("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(softAP_ssid, softAP_password);
  delay(500); // Without delay I've seen the IP address blank
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
    /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", apIP);

  //load ssid/pwd from EEPROM.

  //can connecto to wlan?
  if(strlen(wlanConfig.ssid) > 0) requestWlanConnect();
}

void SoftApCaptivePortal::tick() {  
  //DNS
  dnsServer.processNextRequest();
  if(wlanConnected && requestedApStopOn > 0 && (millis() - requestedApStopOn) >= 30000) {    
    WiFi.softAPdisconnect(true);
    requestedApStopOn = 0;
    Serial.println("AP mode disabled!");
  }  
  if(requestedWlanConnectOn > 0 && (millis() - requestedWlanConnectOn) >= 1000) {
    requestedWlanConnectOn = 0;
    if(connectToWifi() == WL_CONNECTED) {
      wlanConnected = true;
      requestApStop(); 
    }    
  }  
}
boolean SoftApCaptivePortal::requestedWlanConnect(){
  return requestedWlanConnectOn > 0;
}
void SoftApCaptivePortal::requestWlanConnect() {
  requestedWlanConnectOn = millis();
  requestedWlanConnectOn = requestedWlanConnectOn == 0 ? 1: requestedWlanConnectOn;    
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean SoftApCaptivePortal::redirectToPortal() {
  //if (!isIp(server->hostHeader()) && server->hostHeader() != (String(myHostname) + ".local")) {
  if(server->hostHeader() != (String(myHostname) + ".lan.vinzz.be")) {
    WiFiClient client = server->client();
    if(client == NULL) {
      Serial.println("No client in AP-CP request");      
      return true; //do not handle this request. Just skip it.
    }
    Serial.print("Request ");
    Serial.print(server->uri());
    Serial.println(" redirected to captive portal");
    server->sendHeader("Location", String("http://") + toStringIp(client.localIP()), true);
    server->send(302, "text/plain", "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
  //  server->client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

/** Wifi config page handler */
void SoftApCaptivePortal::handleWifi() {
  server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server->sendHeader("Pragma", "no-cache");
  server->sendHeader("Expires", "-1");
  int wifiState = WiFi.status();
  if(requestedWlanConnect()) {
    Serial.println("Refresh page ecery 5 sec waiting for wifi result...");
    server->sendHeader("Refresh", "5");
    server->send(200, "text/html", "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><meta charset='utf-8' /><title>HA Switch</title></head><body><h3>Connecting, Please wait...</h3></body></html>");
    return;
  }
  if(wlanConnected) {
    String Page = String(F("<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><meta charset='utf-8' /><title>HA Switch</title></head><body>Your HA-Switch succesfully connected to network with SSID: ")) 
    + wlanConfig.ssid + F("<br />Connect your device to this WiFi network and visit <a targer='_BLANK' href='http://") 
    + toStringIp(WiFi.localIP()) + F("'>") + toStringIp(WiFi.localIP()) 
    + F("</a> for futher configuration.<br />All connections to this temporary Access Point will be terminated in a few seconds...</body></html>");
    server->send(200, "text/html", Page );
    return;  
  }

  String Page;
  Page += F(
            "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><meta charset='utf-8' /><title>HA Switch</title></head><body>"
            "<h1>Wifi config</h1>");
 //if (server->client().localIP() == apIP) {
    Page += String(F("<p>You are connected through the soft AP: ")) + softAP_ssid + F("</p>");
//  } else {
//    Page += String(F("<p>You are connected through the wifi network: ")) + wlanConfig.ssid + F("</p>");
//  }
  Page +=
    String(F(
             "\r\n<br />"
             "<table><tr><th align='left'>SoftAP config</th></tr>"
             "<tr><td>SSID ")) +
    String(softAP_ssid) +
    F("</td></tr>"
      "<tr><td>IP ") +
    toStringIp(WiFi.softAPIP()) +
    F("</td></tr>"
      "</table>"
      "\r\n<br />"
      "<table><tr><th align='left'>WLAN config</th></tr>"
      "<tr><td>SSID ") +
    String(wlanConfig.ssid) +
    F("</td></tr>"
      "<tr><td>IP ") +
    toStringIp(WiFi.localIP()) +
    F("</td></tr>"
      "</table>"
      "\r\n<br /><h4>Connect to network:</h4><form method='POST' action='/wifisave'><table><tr><th align='left'>WLAN list (<a href='/'>refresh if any missing</a>)</th></tr>");
  int n = WiFi.scanNetworks();
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      Page += String(F("\r\n<tr><td><input id='r")) + i + F("' type='radio' name='n' value='") + htmlEntities(WiFi.SSID(i)) + F("'><label for='r") + i + F("'>") + htmlEntities(WiFi.SSID(i)) + ((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? F(" ") : F(" *")) + F(" (") + WiFi.RSSI(i) + F(")</label></td></tr>");
    }
  } else {
    Page += F("<tr><td>No WLAN found</td></tr>");
  }
  Page += F(
            "\r\n<tr><td><input type='radio' name='n' value=''>Manual:"
            "<input type='text' placeholder='network' name='mn'/></td></tr>"
            "</table>"
            "<br /><input type='password' placeholder='password' name='p'/>"
            "<br /><input type='submit' value='Connect'/></form>"            
            "</body></html>");
  server->send(200, "text/html", Page);
  //server->client().stop(); // Stop is needed because we sent no content length
  Serial.println("handleWifi executed");
}

void SoftApCaptivePortal::requestApStop() {
  Serial.println("Received request to disable the AP mode.");
  requestedApStopOn = millis();
  requestedApStopOn = requestedApStopOn == 0 ? 1 : requestedApStopOn;
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void SoftApCaptivePortal::handleWifiSave() {  
  Serial.println("wifi save");
  String ssid = server->arg("n");
  if(ssid == "")
    ssid = server->arg("mn");
  ssid.toCharArray(wlanConfig.ssid, 32);
  server->arg("p").toCharArray(wlanConfig.password, 32);
  //request for WLAN connection  (It is not possible to initialize the WLAN connection and then send response to AP client. The connection will be gone...
  //We need to send the response first, then connect to wifi...
  //connectWLAN = strlen(wlanConfig.ssid) > 0;
  wlanConnected = false;
  if(strlen(wlanConfig.ssid) > 0) requestWlanConnect();  
//  int wifiStatus = connectToWifi();
// 
//  
//  if(wifiStatus == WL_CONNECTED) {
//    String Page = String(F("<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><meta charset='utf-8' /><title>HA Switch</title></head><body>Your HA-Switch succesfully connected to network with SSID: ")) 
//    + ssid + F("<br />Connect your device to this WiFi network and visit <a href='http://") 
//    + toStringIp(WiFi.localIP()) + F("'>") + toStringIp(WiFi.localIP()) 
//    + F("</a> for futher configuration.<br />Active connections to this Access Point will be terminated in a few seconds...</body></html>");
//    Serial.println(Page);
//    client.send(200, "text/html", Page );
//    requestApStop(); 
//    return;
//  }  
//  Serial.print("Connection failed! WiFi status: ");
//  Serial.println(wifiStatus);
//  connectWLAN = false;    

  //Always redirect to CP page.
  server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server->sendHeader("Pragma", "no-cache");
  server->sendHeader("Expires", "-1");   
  server->sendHeader("Location", "/", true);
  server->send(302, "text/plain", "");
  
  //saveCredentials();
  
//  if(connectToWifi() == WL_CONNECTED)
//    requestApStop();  
//  else
//    connectWLAN = false;    
}

String SoftApCaptivePortal::htmlEntities(String str) {
  if(str.length() > 0) {
    str.replace("&","&amp;");
    str.replace("<","&lt;");
    str.replace(">","&gt;");
    str.replace("'","&#39;");
    str.replace("\"","&quot;");
    // str.replace("/": "&#x2F;");
    // str.replace("`": "&#x60;");
    // str.replace("=": "&#x3D;");
  }
  return str;
}

/** Is this an IP? */
boolean SoftApCaptivePortal::isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) 
      return false;    
  }
  return true;
}

/** IP to String? */
String SoftApCaptivePortal::toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++)
    res += String((ip >> (8 * i)) & 0xFF) + ".";  
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}
