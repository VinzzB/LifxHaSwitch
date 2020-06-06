#include "Arduino.h"
#include "MotionDetector.h"


void MotionDetector::setup(byte gpioPin) {
  IInputDevice::setup(gpioPin);
  prevState = digitalRead(gpioPin);
}

int MotionDetector::check(){
  boolean buttonVal = digitalRead(gpioPin);  
  if(prevState != buttonVal) {
      Serial.print("MotionDetector on gpio ");
      Serial.print(gpioPin);
      Serial.print(" changed to ");
      Serial.println(buttonVal);
      prevState = buttonVal;
      return buttonVal ? ACTION_ON : ACTION_OFF;
  }
  return ACTION_NONE;
}
