#include <ArduTone.h>
//Connect a speaker or buzzer to the PIN 5 and GND on tour arduino board
ArduTone Tone(5,"MFT240L4 O4aO5dc O4aO5dc O4aO5dc L16dcdcdcdc");
void setup() {
 Serial.begin(115200);
 Serial.println("ArduPiano Start");
}

void loop() {
  // put your main code here, to run repeatedly:
Tone.start();
delay(10000);
}