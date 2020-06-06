#ifndef MotionDetector_h
  #define MotionDetector_h
  #include "Arduino.h"
  #include "SwitchTypen.h" 
  #include "IInputDevice.h"
  
  class MotionDetector : public IInputDevice {
    public:
      void setup(byte gpioPin);
      int check();
    private:
      bool prevState;
  };
  
#endif
