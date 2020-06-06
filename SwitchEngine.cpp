#include "Arduino.h"
#include "SwitchEngine.h"


//byref: LifxDevice* updDevice = devices[x];
//copy:  LifxDevice device = *devices[x];
    
SwitchEngine::SwitchEngine(const uint8_t *buttonPins, uint8_t buttonPinsSize, MqttService* mqttService) {
  _lifxDevices = new LifxDevice[10];
  _udp.begin(56701);  
  _broadcastIp = IPAddress(255,255,255,255);
  
  this->mqttState = new MqttState[buttonPinsSize]; //todo : to buttonPins
  //this->pinDevices = new uint8_t[buttonPinsSize];
  this->pinConfig = new PinConfig[buttonPinsSize];
    //Serial.begin(115200); //todo: moet hier niet (is om global vars te debuggen)
    for(int x = 0; x < 4; x++) {
      this->pinConfig[x].name[0] = '\0';
//    Serial.print("Switch ")  ;
//    Serial.print(x+1);
//    Serial.print(" - Name: ");
//    Serial.println(pinConfig[x].name);
  }
  this->buttonPins = buttonPins;
  this->buttonPinsSize = buttonPinsSize;
  this->buttons = new IInputDevice*[buttonPinsSize];
  this->mqttService = mqttService;
  this->setPinDevices();
//  for(int x = 0; x < buttonPinsSize; x++) {
//    if(pinConfig[x].type == 0) {
//      this->buttons[x] = new MultiButton();
//      buttons[x]->setup(buttonPins[x]);
//    }
//    if(pinConfig[x].type == 1) {
//      this->buttons[x] = new MotionDetector();
//      buttons[x]->setup(buttonPins[x]);
//    }
//  }
}

void SwitchEngine::setPinDevices() {
   for(int x = 0; x < buttonPinsSize; x++) {
    if(pinConfig[x].type == 0) {
      buttons[x] = new MultiButton();
      buttons[x]->setup(buttonPins[x]);
    }
    if(pinConfig[x].type == 1) {
      buttons[x] = new MotionDetector();
      buttons[x]->setup(buttonPins[x]);
    }
  } 
}

void SwitchEngine::setBroadcastIp(IPAddress broadcastIp) {
  _broadcastIp = broadcastIp;
  searchDevices();
}

uint8_t SwitchEngine::countFoundDevices() {
  return _lifxDevicesFoundCounter;  
}

LifxDevice &SwitchEngine::elementAt(uint8_t index) {
  return _lifxDevices[index];
}

LifxDevice* &SwitchEngine::devices() {
  return _lifxDevices;  
}

bool SwitchEngine::devicesOnSwitch(byte pinIdx, LifxDevice** devices, byte devicesSize) {
  int currIdx = 0;
  bool allUpToDate = true;
  for(int x = 0; x < _lifxDevicesFoundCounter; x++) {
    LifxDevice* device = &_lifxDevices[x];
    if(bitRead(device->groups.raw[0], pinIdx)) {
      devices[ currIdx++ ] = device;  
      //is bulb up to date?
      allUpToDate = millis() < device->lastUpdateTime + 10000;
    }
  }
  return allUpToDate;
}

byte SwitchEngine::countDevicesOnSwitch(byte pinIdx) {
  int currIdx = 0;
  for(int x = 0; x < _lifxDevicesFoundCounter; x++)
    if(bitRead(_lifxDevices[x].groups.raw[0], pinIdx))
      currIdx++;  
  return currIdx;
}

void SwitchEngine::executeMqttAction(byte pinIdx, bool setValue) {
  switch(queuedActions[pinIdx]) {
    case ACTION_TOGGLE: {
      if(setValue)
        mqttState[pinIdx].powerState = !mqttState[pinIdx].powerState;
      String topic = "powerState/" + String(pinIdx,DEC);
      int topic_len = topic.length() + 1;
      char topicChars[topic_len];
      topic.toCharArray(topicChars, topic_len);
      //todo aanpasbare prefix voorzien
      mqttService->sendCommand(topicChars, mqttState[pinIdx].powerState ? "ON" : "OFF"); 
      
      break;
    }
  }
}

void SwitchEngine::mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  char cpayload[length+1];
  for (int i = 0; i < length; i++) {    
    cpayload[i] = (char)payload[i];
    //Serial.print((char)payload[i]);
  }
  cpayload[length] = NULL;  
  Serial.println(cpayload);
  char powerState[] = "powerState/";
  if(memcmp(topic, powerState, sizeof(powerState)-1) == 0) {
    byte pinIdx = atoi(topic+11);
    if(pinIdx < 8) {      
      mqttState[pinIdx].powerState = strcmp(cpayload, "ON") == 0;
      Serial.print("set powerstate on pinIdx ");
      Serial.print(pinIdx);
      Serial.print(" - value: ");
      Serial.println(mqttState[pinIdx].powerState);
    }
  }

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

bool SwitchEngine::executeAction(byte pinIdx) {
  
  //get device count on switch
  byte devicesSize = countDevicesOnSwitch(pinIdx);
  if(devicesSize == 0) {
    executeMqttAction(pinIdx, true);
    queuedActions[pinIdx] = 0;
    return false;
  }
  
  //get devices bound to switch and check if an update is needed prior to executing.
  LifxDevice* devices[devicesSize];  
  bool areUpToDate = devicesOnSwitch(pinIdx, devices, devicesSize);  
  if(!areUpToDate && queuedActions[pinIdx] == ACTION_TOGGLE) {    
    for(int x = 0; x < devicesSize;x++){
      LifxDevice* updDevice = devices[x];
      if(updDevice->updateRequestedTime + 1000 < millis()) {
        Serial.print("Not all devices are up to date. requesting update for device: ");
        Serial.println(updDevice->name);
        updDevice->updateRequestedTime = millis();
        updateDevice(&updDevice->ip);
      }
    }    
    return false;
  }
  //execute the action
  switch(queuedActions[pinIdx]) {
    case ACTION_TOGGLE: {  //QUEUE_ACTION_POWER_TOGGLE
      togglePower(pinIdx, devices, devicesSize);      
      break; 
    }       
    case 3: {  //QUEUE_ACTION_BRIGHTNESS_FADE

      break;
    }
    case ACTION_ON: {
      setPower(pinIdx, devices, devicesSize, true);
      break;  
    }
    case ACTION_OFF: {
      setPower(pinIdx, devices, devicesSize, false);
      break;  
    }
  }
}

//todo naar aparte lifx clsaa!
LifxPacket createLifxPacket(uint16_t type, uint16_t packetSize) {
   LifxPacket packet = LifxPacket();          
    packet.protocol = 1024;
    packet.type = type; // SET_POWER_STATE; // SET_POWER_STATE; // GET_PAN_GATEWAY;
    packet.addressable = true;
    packet.source = 6666;
    packet.origin = 0;
    packet.sequence = 100;
    packet.tagged = true;
    packet.size = packetSize;
  return packet;
}
void SwitchEngine::setPower(byte pinIdx, LifxDevice** devices, byte devicesSize, bool powerState ) {
  mqttState[pinIdx].powerState = powerState;
  executeMqttAction(pinIdx, false);
  for(int x = 0; x < devicesSize; x++) {          
    LifxDevice* device = devices[x]; //LifxDevice device = *devices[x];
    device->powerState = powerState;
    
    LifxPacket pwrRequest = createLifxPacket(SET_POWER_STATE,42); // LifxPacket();          
//    pwrRequest.protocol = 1024;
//    pwrRequest.type = SET_POWER_STATE; // SET_POWER_STATE; // GET_PAN_GATEWAY;
//    pwrRequest.addressable = true;
//    pwrRequest.source = 6666;
//    pwrRequest.origin = 0;
//    pwrRequest.sequence = 100;
//    pwrRequest.tagged = true;
//    pwrRequest.size = 42;

    Serial.print("Sending power command to ip: ");
    Serial.println(device->ip);
    Serial.print("REQ: ");
    byte payload[6];    
    payload[0] = payload[1] = powerState ? 0xFF : 0x0;
    uint32_t duration = 250;
    memcpy(payload+2, &duration,4);    
    _printLifxPacket(pwrRequest,payload, 6);
    _udp.beginPacket(device->ip, 56700);
    _udp.write(pwrRequest.raw, 36); 
    _udp.write(payload, 6); 
    _udp.endPacket();         
  }
}

void SwitchEngine::togglePower(byte pinIdx, LifxDevice** devices, byte devicesSize ) {
    queuedActions[pinIdx] = 0; //Reset queue
  //togglePower
  bool anyHasPower = false;
  for(int x = 0; x < devicesSize; x++) {
    anyHasPower = devices[x]->powerState;
    if(anyHasPower)
      break;
  }
  //now lets toggle!
  Serial.print("Sending power command: ");
  Serial.print(!anyHasPower);
  Serial.print(" requested by switch ");
  Serial.println(pinIdx+1);        
  //MQTT
  this->setPower(pinIdx, devices, devicesSize, !anyHasPower);
//  mqttState[pinIdx].powerState = !anyHasPower;
//  executeMqttAction(pinIdx, false);
//  for(int x = 0; x < devicesSize; x++) {          
//    LifxDevice* device = devices[x]; //LifxDevice device = *devices[x];
//    device->powerState = !anyHasPower;
//    
//    LifxPacket pwrRequest = createLifxPacket(SET_POWER_STATE,42); // LifxPacket();          
////    pwrRequest.protocol = 1024;
////    pwrRequest.type = SET_POWER_STATE; // SET_POWER_STATE; // GET_PAN_GATEWAY;
////    pwrRequest.addressable = true;
////    pwrRequest.source = 6666;
////    pwrRequest.origin = 0;
////    pwrRequest.sequence = 100;
////    pwrRequest.tagged = true;
////    pwrRequest.size = 42;
//
//    Serial.print("Sending power command to ip: ");
//    Serial.println(device->ip);
//    Serial.print("REQ: ");
//    byte payload[6];    
//    payload[0] = payload[1] = anyHasPower ? 0x0 : 0xFF;
//    uint32_t duration = 250;
//    memcpy(payload+2, &duration,4);    
//    _printLifxPacket(pwrRequest,payload, 6);
//    _udp.beginPacket(device->ip, 56700);
//    _udp.write(pwrRequest.raw, 36); 
//    _udp.write(payload, 6); 
//    _udp.endPacket();         
//  }
}

void SwitchEngine::tick() {
  //read udp packets (if any)  
  readUdpPacket();  
  //read buttons.
   for(int x = 0; x < buttonPinsSize; x++) {        
    byte btnState = buttons[x]->check();
    //push action onto queue.
    if(queuedActions[x] == 0 || btnState > 0) {
      queuedActions[x] = btnState;
    }
    //reset queue when button was held down and got released.
    if(queuedActions[x] > 1 && btnState == 0)
       queuedActions[x] = 0;
       
   
    //debug...
    switch(queuedActions[x]) {
      case 1: { //single click
        Serial.print("Switch ");  Serial.print(x+1); Serial.println(" pressed once"); break;
      } 
      case 2: { //double click
        Serial.print("Switch "); Serial.print(x+1); Serial.println(" pressed twice");  queuedActions[x] = 0; break;
      } 
      case 3: { //hold
        Serial.print("Switch "); Serial.print(x+1); Serial.println(" pressed long"); break;
      }  
      case 4: { //20sec hold
        Serial.print("Switch ");  Serial.print(x+1); Serial.println(" pressed 20s"); break;
      }
    }
    if(queuedActions[x] > 0)
      executeAction(x);            
  }  
}

bool SwitchEngine::readUdpPacket() {
   // if there's data available, read a packet
  int packetSize = _udp.parsePacket();
  if (packetSize) {   
    Serial.printf("Received packet of size %d from %s:%d\n    (to %s:%d, free heap = %d B)\n",
                  packetSize,
                  _udp.remoteIP().toString().c_str(), _udp.remotePort(),
                  _udp.destinationIP().toString().c_str(), _udp.localPort(),
                  ESP.getFreeHeap());
                      
    LifxPacket resp;
    LifxDevice* device;
    bool deviceExist = false;
    byte payload[packetSize - 36];  
        
    //load Frame / Frame Address & Protocol header.
    _udp.read(resp.raw, 36);
    //Load payload into byte array.
    _udp.read(payload, packetSize - 36);
    
    //DEBUG INFO
    Serial.print("     RSP: ");
    _printLifxPacket(resp, payload, packetSize - 36);

    //get or add device to internal list
    for(int x = 0; x < 10; x++) {
      deviceExist = _macEquals(_lifxDevices[x].mac, resp.target);
      if(deviceExist) {
        device = &_lifxDevices[x];
        break;
      }
    }    
    
    //create and add device to list if needed
    if(!deviceExist){
      if(_lifxDevicesFoundCounter >= 10)
        return false;
      //create a LifxDevice 
      _lifxDevices[_lifxDevicesFoundCounter] = LifxDevice();
      device = &_lifxDevices[_lifxDevicesFoundCounter++];
      memcpy(device->mac, resp.target, 6);
    }
    
    //copy available props in response to device object.
    device->ip = _udp.remoteIP();
    if(resp.type == LIGHT_STATUS) {
      memcpy(device->name, payload+12, 32);
      memcpy(&device->colorState, payload, 8);
      device->powerState = payload[10] > 0;
    }
    if(resp.type == 22) {      
      device->powerState = payload[0] > 0; 
    }    
    device->lastUpdateTime = millis();  
  }  
  return packetSize > 0;
}

bool SwitchEngine::_macEquals(byte *array1, byte *array2) {
   for (int n=0;n<6;n++) if (array1[n]!= array2[n]) return false;
}

//void SwitchEngine::updateDevices(IPAddress** adresses, byte adressesSize) {
void SwitchEngine::updateDevice(IPAddress* address) {  
    IPAddress ip = *address;    
    byte payload[0];    
    LifxPacket discoveryRequest = createLifxPacket(GET_LIGHT_STATE, 36);
//    discoveryRequest.protocol = 1024;
//    discoveryRequest.type = GET_LIGHT_STATE; // GET_PAN_GATEWAY;
//    discoveryRequest.addressable = true;
//    discoveryRequest.source = 2468;
//    discoveryRequest.sequence = 1;
//    discoveryRequest.tagged = true;
//    discoveryRequest.size = 36;
    
    Serial.print("request update for device on ip: ");
    Serial.println(ip);
    Serial.print("REQ: ");       
    _printLifxPacket(discoveryRequest,payload, 0);
    
    _udp.beginPacket(ip, 56700);
    _udp.write(discoveryRequest.raw, 36); 
    _udp.endPacket();  
}

void SwitchEngine::searchDevices() {  
  updateDevice(&_broadcastIp);
}

void SwitchEngine::_printLifxPacket(LifxPacket &request, byte payload[], int payloadsize) {
    uint8_t i = 0;

   //print lifx packet in hex
    for(i = 0; i < 36; i++) {
      Serial.print(request.raw[i], HEX);
      Serial.print((" "));
    }

    //print payload in hex
    Serial.print((" "));
    for(i = 0; i < payloadsize; i++) {
      Serial.print(payload[i], HEX);
      Serial.print((" "));
    }  

//    Serial.print(Udp.remoteIP()); //todo: broadcast or unicast...
//    Serial.print((":"));    
//    Serial.print(Udp.remotePort()); //todo: will change when implementing decent broadcasts
    
    Serial.print(("\r\n    | Size:"));
    Serial.println(request.size);
    
    Serial.print(("    | Proto: "));
    Serial.println(request.protocol);

//    Serial.print((" (0x"));
//    Serial.print(request.raw[2], HEX);
//    Serial.print((" "));
//    Serial.print(request.raw[3], HEX);
//    Serial.print((" "));
//    Serial.print((")"));
    
    Serial.print(("    | addressable: "));
    Serial.println(request.addressable);
    
    Serial.print(("    | tagged: "));
    Serial.println(request.tagged);

    Serial.print(("    | origin: "));
    Serial.println(request.origin);

    Serial.print(("    | source: 0x"));
    Serial.println(request.source, HEX);

    Serial.print(("    | target: 0x"));
    for(i = 0; i < 8; i++) {
      Serial.print(request.target[i], HEX);
      Serial.print((" "));
    }
    
    Serial.print(("\r\n    | reserved1: 0x"));
    for(i = 0; i < 6; i++) {
      Serial.print(request.reserved1[i], HEX);
      Serial.print((" "));
    }
    
    Serial.print(("\r\n    | res_required:"));
    Serial.println(request.res_required);

    Serial.print(("    | ack_required:"));
    Serial.println(request.ack_required);

    Serial.print(("    | reserved2: 0x"));
    Serial.println(request.reserved2, HEX);

    Serial.print(("    | sequence: 0x"));
    Serial.println(request.sequence, HEX);
    
//    Serial.print(F(" | reserved3: 0x"));
//    Serial.print(request.reserved3, HEX);
    
    Serial.print(("    | type: 0x"));
    Serial.print(request.type, HEX); 
    Serial.print(" (");
    Serial.print(request.type);
    Serial.println(")"); 
     
    Serial.print(("    | reserved4: 0x"));
    Serial.println(request.reserved4, HEX);

//    Serial.print((" | data: "));
//    if(request.type == 510 || request.type == 512)
//      Serial.print(("< Stream >"));
  //  else {
   //   for(i = 0; i < request.data_size; i++) {
  //      Serial.print(reqData[i], HEX);
  //      Serial.print(F(" "));
  //    }    
   // }
        
//    Serial.print(F(" | data_size:"));
//    Serial.print(request.data_size);
  //  Serial.println();  
}
