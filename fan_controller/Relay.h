// Relay.h

#ifndef _RELAY_h
#define _RELAY_h

#include "Arduino.h"

#define _R_NORMALLY_OPEN    true
#define _R_NORMALLY_CLOSE   false
  
class Relay {
 protected:
	 int pin;
	 bool state;
	 bool normallyOpen;

 public:
	 Relay();
	 Relay(int p, bool isNormallyOpen);
     void setPin( int p );
     void setNormally( bool isNormallyOpen );
	 void begin();
	 bool getState();
	 void turnOn();
	 void turnOff();
};

#endif
