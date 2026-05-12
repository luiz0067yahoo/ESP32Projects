#include <SPI.h>
#include <RF24.h>
#include <Servo.h>

#define CE_PIN 9   // Use appropriate pin for your board
#define CSN_PIN 10 // Use appropriate pin for your board
#define Servo_PIN 16
RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";
char receivedText[32] = ""; // Buffer to hold received data
Servo myservo;
int servoPin = 16;

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_1MBPS);
  radio.startListening();
  myservo.attach(servoPin);
}

void loop() {
  if (radio.available()) {
    radio.read(&receivedText, sizeof(receivedText));
    Serial.print("Received data: ");
    Serial.println(receivedText);
    if (sizeof(receivedText) >= 1){
      myservo.write(0);   // Turn to 0 degrees
      delay(100);
      myservo.write(90);  // Turn to 90 degrees
      delay(100);
      myservo.write(180); // Turn to 180 degrees
      delay(100);
    }
  }
}