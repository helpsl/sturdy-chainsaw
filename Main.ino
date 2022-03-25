
#include "OutputDevice.h"
#include "InputDevice.h"

OutputDevice output;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);  //  set the serial monitor at 115200 too

  output.Init();

#if !defined(__MIPSEL__)
  // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
  while (!Serial && (millis() < 3000));
#endif
  Serial.println(F("Start"));

  input.init();
}

void loop() {  
  // put your main code here, to run repeatedly:
  input.task();
  output.SendState();
  delay(1);
}
