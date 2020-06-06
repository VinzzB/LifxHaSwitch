#ifndef IInputDevice_h
#define IInputDevice_h
#include "Arduino.h"

class IInputDevice {
  public:
    virtual void setup(byte gpioPin) {
      this->gpioPin = gpioPin;  
      pinMode(gpioPin, INPUT); //todo set to pullup
    };
    //void sendCommand(const char* topic, const char* value);
    virtual int check()=0;
  protected:
   byte gpioPin;
};
#endif
