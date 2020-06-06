#include "Arduino.h"
#include "SwitchServer.h"

SwitchServer::SwitchServer(BearSSL::ESP8266WebServerSecure *server, SwitchEngine* switchEngine) {
  _switchEngine = switchEngine;
  _server = server;
  webTemplateStart = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><meta charset='utf-8' /><title>HA Switch</title><style>body{margin:50px;}input[type=checkbox],table{width:100%;text-align:center;}th,td{border-bottom:1px solid #ddd;}tr.row:hover{background-color:#ddd;}</style></head><body>";
  webTemplateEnd = "</body></html>";
}

void SwitchServer::handleRoot() {
  if(_server->method() == HTTP_POST)
    postRoot();
  else
    getRoot();
}
/*
- MQTT Server
- MQTT prefix
- Hostname?
- 

*/
void SwitchServer::getRoot() {   
   Serial.println("Sending root");
  _server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  _server->send ( 200, "text/html", webTemplateStart);
  _server->sendContent("<h1>Lifx switch configuratie</h1>");
  _server->sendContent("<h3>Pin Configuration</h3><form method='POST'><table><thead><tr><th>#</th><th>Name</th><th>Input Type</th><th>GPIO Pin</th></tr></thead><tbody>");  
  for(int x = 0; x < _switchEngine->buttonPinsSize; x++) {
      _server->sendContent("<tr class='row'><td>");
      _server->sendContent((String)(x+1));
      _server->sendContent("</td><td>");
      //name
      if(_switchEngine->pinConfig[x].name[0] == '\0')
      ("Switch " + String(x+1)).toCharArray(_switchEngine->pinConfig[x].name,32);
        //_switchEngine->pinConfig[x].name = "Switch " + String(x);
      this->sendInputBox("pinName_" + (String)x, _switchEngine->pinConfig[x].name);
      _server->sendContent("</td><td>");  
      this->sendDeviceSelectBox(x, _switchEngine->pinConfig[x].type);
      _server->sendContent("</td><td>");
      //name
      _server->sendContent((String)_switchEngine->buttonPins[x]);
      _server->sendContent("</td><td></tr>");  
  }  
  _server->sendContent("</tbody></table><h3>Lifx devices found on network:</h3> <form method='POST'><table><thead><tr><th>#</th><th>Name</th><th>MAC</th><th>Ip Address</th>");
  for(int x = 0; x < _switchEngine->buttonPinsSize; x++) {
    _server->sendContent("<th>");
    _server->sendContent(htmlEntities(_switchEngine->pinConfig[x].name));
    //_server->sendContent((String)(x+1));
   // this->sendDeviceSelectBox(x, _switchEngine->pinDevices[x]);
    _server->sendContent("</th>");    
  }
  _server->sendContent("</tr></thead><tbody>");  
  for(int x = 0; x < _switchEngine->countFoundDevices(); x++) {
    LifxDevice device = _switchEngine->elementAt(x);
    //index
    _server->sendContent("<tr class='row'><td>");
    _server->sendContent((String)(x+1));
    _server->sendContent("</td><td>");
    //name
    _server->sendContent(htmlEntities(device.name));
    _server->sendContent("</td><td>");
    //mac    
    for(int m = 0; m < 6; m++) {
      _server->sendContent(String(device.mac[m], HEX) + (m < 6-1 ? ":" : ""));
    }
    _server->sendContent("</td><td>");
    //ip
    _server->sendContent(device.ip.toString().c_str());
    _server->sendContent("</td>");
    //checkboxes for switches
    for(int p = 0; p < _switchEngine->buttonPinsSize; p++) {
      _server->sendContent("<td>");
      //_server->sendContent(createCheckbox("selDev",String(x) + "_" + String(p), bitRead(device.groups.raw[0], p))); // "<input type='checkbox' name='selDevIdx' value='";
      this->sendCheckbox("selDev",String(x) + "_" + String(p), bitRead(device.groups.raw[0], p)); // "<input type='checkbox' name='selDevIdx' value='";
      _server->sendContent("</td>");
    }
    _server->sendContent("</tr>");
  }
  _server->sendContent("</tbody></table><br /><input type='submit' name='action' value='Save'> <input type='submit' name='action' value='Save in EEPROM'></form>");
  _server->sendContent("<form method='POST'><input type='submit' formAction='/discover' value='Discover Lifx devices'><input type='submit' formAction='/reset' value='Factory reset'></form>");
  _server->sendContent(webTemplateEnd); 
  //_server->client().stop();
   Serial.println("Root send ");
}

void SwitchServer::postRoot() {
    //handle data
    String msg = "POST form was:\n";
    //clear selection
    for(int i = 0; i < _switchEngine->countFoundDevices(); i++) {
      LifxDevice &device = _switchEngine->elementAt( i );
      device.groups.raw[0] = 0;
    }
    //set selection  
    for (uint8_t i = 0; i < _server->args(); i++) {      
      String argName = _server->argName(i);
      String argValue = _server->arg(i); 
      msg += " " + argName + ": " + argValue + "\n";         
      if(argName == "selDev") {
        int currDevIdx = argValue.substring(0,argValue.indexOf("_")).toInt();
        int currSwIdx = argValue.substring(argValue.indexOf("_")+1).toInt();
        if(currDevIdx >= 0 && currDevIdx < _switchEngine->countFoundDevices()) {
          LifxDevice &device = _switchEngine->elementAt( currDevIdx );
          bitWrite(device.groups.raw[0],currSwIdx ,true);
        }
      } 
      if(argName.startsWith("deviceType_")) {
        int currPinIdx = argName.substring(11).toInt();
        _switchEngine->pinConfig[currPinIdx].type = argValue.toInt();
      }
      if(argName.startsWith("pinName_") && argValue.length() < 33) {
        int currPinIdx = argName.substring(8).toInt();
        argValue.toCharArray(_switchEngine->pinConfig[currPinIdx].name,33);
      }      
      if(argName == "action" && argValue == "Save in EEPROM") {
        Serial.println("CONFIG SAVED TO EEPROM!!!");
      }
    }
    Serial.println(msg);
    _switchEngine->setPinDevices();
    //respond with a redirect to root.
    _server->sendHeader("Location", _server->uri(), true);
    _server->send ( 302, "text/plain", "");
    //_server->client().stop();
}

void SwitchServer::sendCheckbox(String name, String value, bool checked) {
  this->sendInput("checkbox", name, value, checked);
}

void SwitchServer::sendInputBox(String name, String value) {
  this->sendInput("text", name, value, false);
}

void SwitchServer::sendInput(const char* type, String name, String value, bool checkboxChecked) {
  _server->sendContent("<input type='");
  _server->sendContent(type);
  _server->sendContent("' name='");
  _server->sendContent(name);
  _server->sendContent("' value='");
  //SANITIZE INPUTS!
  //allow from DEC 48 (= 0) t/m 57 (=9) of 65 (=A) t/m 90 of 97 t/m 122   EN 32 (space)
  
  //OFWEL: 32 || 40 t/m 59 || 63 t/m 126
  if(value.length() > 0) {
    value.replace("'","&#39;");
    _server->sendContent(value);
}
  _server->sendContent("' ");
  if(checkboxChecked)
    _server->sendContent(" checked");
  _server->sendContent(">");
}

String SwitchServer::htmlEntities(String str) {
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

void SwitchServer::sendDeviceSelectBox(const byte pinIdx, uint8_t value) {

  _server->sendContent("<select name='deviceType_");
  _server->sendContent((String)pinIdx);
  _server->sendContent("'><option value='0'");
  if(value == 0)
    _server->sendContent(" selected");
  _server->sendContent(">Button</option><option value='1'");
  if(value == 1)
    _server->sendContent(" selected");
  _server->sendContent(">Motion detector</option></select>");
}

void SwitchServer::handleNotFound() { 
  Serial.print("Endpoint undefined: ");
  Serial.println(_server->uri());
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += _server->uri();
  message += "\nMethod: ";
  message += (_server->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += _server->args();
  message += "\n";
  for (uint8_t i = 0; i < _server->args(); i++) {
    message += " " + _server->argName(i) + ": " + _server->arg(i) + "\n";
  }
  _server->send(404, "text/plain", message);
}

void SwitchServer::handleFavicon() {
  
}

void SwitchServer::handleLifxDiscovery() {
  if(_server->method() == HTTP_POST) {
    _switchEngine->searchDevices();
    _server->sendHeader("Location", "/", true);
    _server->send ( 302, "text/plain", "");
    //_server->client().stop();
  }
  
  //create json string containing discovered lifx devices
//  if(_server->method() == HTTP_GET) {
//    String msg = "[";
//    for(int x = 0; x < _switchEngine->countFoundDevices(); x++) {
//      LifxDevice &device = _switchEngine->elementAt( x );
//      msg += "{";
//      msg += "\"ip\" : \"";
//      msg += device.ip.toString().c_str();
//      msg += "\",\"";
//      msg += "mac\" : \"";
//      for(int m = 0; m < 6; m++) {
//        msg += String(device.mac[m], HEX) + (m < 6-1 ? ":" : "");
//      }
//      msg += "\"";
//      //msg += lifxDevices[x].ip.toString().c_str();
//      msg += "}";
//      msg += x < _switchEngine->countFoundDevices()-1 ? "," : "";
//    }
//    msg += "]";
//    _server->send(200, "application/json", msg);
//  } else {
//    _server->send(500, "text/plain", "Error: Invalid command!");
//  }
}
