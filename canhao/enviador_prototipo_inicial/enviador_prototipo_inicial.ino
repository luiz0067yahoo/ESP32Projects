#include <Servo.h>

Servo s;

void setup() {
  s.attach(14); // GPIO14 = D5
}

void loop() {
  s.write(0);
  delay(1000);

  s.write(180);
  delay(1000);
}