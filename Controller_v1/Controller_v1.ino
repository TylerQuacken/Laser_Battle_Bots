/*
 * Laser Battle Bot - Controller v2.0
 * 
 * by Landon Willey
 * 2/8/2018
 * 
 * Library: TMRh20/RF24
 * 
 * PINS:
 * 
 */

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// Create an instance of the radio transmission
RF24 radio(7, 8); // numbers define CE CSN pins
const byte out_address[6] = "00001";
const byte in_address[6] = "00002";

// Declare x and y input pins
byte y_pin = 0;
byte x_pin = 1;

const byte button0 = 3;   //right button
const byte button1 = 2;   //left button
const byte button2 = 4;   //shooter button

// Declare a variable to store all four button states
byte buttons = 0;

void setup() {
  Serial.begin(9600); // Set serial transmission rate
  radio.begin(); // Start the radio
  radio.openWritingPipe(out_address);
  radio.openReadingPipe(2, in_address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening(); // Set as a transmitter

  pinMode(button1, INPUT_PULLUP); // Make buttons digital inputs
  pinMode(button0, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
}

void loop() {

  // Initialize variables to read joystick values
  int y = analogRead(y_pin);
  y = 1023 - y; // Y input needs to be inverted
  int x = analogRead(x_pin);

  // Limit y input to allow for turning at high speeds
  y = map(y, 0, 1023, 127, 895); // 1/8 off the edge of either side
  int scale_factor = 4;

  // Check for deadzone values
  int deadzone = 10;
  if (abs(y-512) < deadzone)
  {
    scale_factor = 2;
    y = 512;
  }
  if (abs(x-512) < deadzone)
    x = 512;

  // Combine inputs to produce motor inputs
  int leftWheel = y + (x-512)/scale_factor;
  int rightWheel = y - (x-512)/scale_factor;

  rightWheel = map(rightWheel, 0, 1023, 0, 255);
  leftWheel = map(leftWheel, -1, 1022, 0, 255);

  // Naturalize joystick directionality for backward motion
  if (rightWheel < 127 && leftWheel < 127)
  {
    int temp = leftWheel;
    leftWheel = rightWheel;
    rightWheel = temp;
  }

  // Use a debouncing algorithm to read the servo command
  int left_1 = digitalRead(button1);
  int right_1 = digitalRead(button0);
  int shooter_1 = digitalRead(button2);
  delay(10);
  
  boolean left = digitalRead(button1) && left_1;
  boolean right = digitalRead(button0) && right_1;
  boolean shooter = digitalRead(button2) && shooter_1;

  bitWrite(buttons,0,right);
  bitWrite(buttons,1,left);
  bitWrite(buttons,2,shooter);
  
  byte message[] = {leftWheel, rightWheel, buttons};
  radio.write(&message, sizeof(message));     //send message, wait for ack

  
    Serial.print(message[0]);
    Serial.print("    ");
    Serial.print(message[1]);
    Serial.print("    ");
    Serial.print(bitRead(message[2],3));
    Serial.print(bitRead(message[2],2));
    Serial.print(bitRead(message[2],1));
    Serial.print(bitRead(message[2],0));
    Serial.println("    ");

    delay(10);
  
//  radio.startListening();               //listen for response
//  while (!radio.available()){           //wait until response comes
//  }
//  char message[32] = "";
//  radio.read(&message, sizeof(message));    //stuff response into message
//  Serial.println(message);
//  delay(1000);
//  radio.stopListening();              //get ready to send again
}



