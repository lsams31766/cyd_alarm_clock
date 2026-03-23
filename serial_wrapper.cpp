#include "Arduino.h"
#include "serial_wrapper.h"

//void initSerial(long baud) { Serial.begin(baud); }
void sendSerial(const char* message) { Serial.println(message); }
