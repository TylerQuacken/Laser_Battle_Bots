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
 * 
 * LiquidCrystal(rs, enable, d4, d5, d6, d7) 
 */

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <LiquidCrystal.h>
#include "control_func.h"
#include "control_chars.h"

int hp = 999;
int hp_max = 999;
int Lammo = 99;
int Lammo_max = 99;

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

#define RS 9
#define EN 10
#define d4 A2
#define d5 A3
#define d6 A4
#define d7 A5

LiquidCrystal lcd(RS, EN, d4, d5, d6, d7);
byte Lgun_char = 0;    //set char numbers for custom chars (0-7)
byte heart_char = 1;   //call lcd.write(XXX_char) to display char
byte shield_char = 2;

byte hp_pos[] = {0,0};     //col, row of position to display each vital
byte Lammo_pos[] = {10,0};

// Declare a variable to store all four button states
byte buttons = 0;

void setup() {
  Serial.begin(9600); // Set serial transmission rate
  radio.begin(); // Start the radio
  radio.openWritingPipe(out_address);
  radio.openReadingPipe(2, in_address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening(); // Set as a transmitter
  
  lcd.begin(16, 2);
  lcd.createChar(Lgun_char, Lgun_bits);
  lcd.createChar(heart_char, heart_bits);
  lcd.createChar(shield_char, shield_bits);
  
  printVitals();    //prints health and ammo to LCD (control_func)

  pinMode(RS, OUTPUT);
  pinMode(EN, OUTPUT);
  pinMode(d4, OUTPUT);
  pinMode(d5, OUTPUT);
  pinMode(d6, OUTPUT);
  pinMode(d7, OUTPUT);
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

  
//    Serial.print(message[0]);
//    Serial.print("    ");
//    Serial.print(message[1]);
//    Serial.print("    ");
//    Serial.print(bitRead(message[2],3));
//    Serial.print(bitRead(message[2],2));
//    Serial.print(bitRead(message[2],1));
//    Serial.print(bitRead(message[2],0));
//    Serial.println("    ");

    printVitals();    //prints health and ammo to LCD (control_func)
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


void printVitals(){

  if (Serial.available() > 0){
  byte incoming0 = Serial.read() - 48;
  byte incoming1 = Serial.read() - 48;
  byte incoming2 = Serial.read() - 48;
  hp = 100*incoming0 + 10*incoming1 + incoming2;
  }
  
  
  lcd.setCursor(hp_pos[0], hp_pos[1]);    //display the hp icon
  lcd.write(shield_char);
  //display current health
  String hp_text;   //will contain 'XXX/XXX'
  if (hp < 100)
    hp_text += " ";   //precede with "_" if hp has fewer digits
  if (hp < 10)
    hp_text += " ";
  hp_text += String(hp) + '/' + String(hp_max);   //concatenate  strings
  lcd.setCursor(hp_pos[0]+1, hp_pos[1]);    
  lcd.print(hp_text);
  
  lcd.setCursor(Lammo_pos[0], Lammo_pos[1]);    //display the light gun icon
  lcd.write(Lgun_char);
  //display current health
  String Lammo_text;   //will contain 'XX/XX'
  if (Lammo < 10)   //precede with "_" if fewer digits
    Lammo_text += " ";
  Lammo_text += String(Lammo) + '/' + String(Lammo_max);   //concatenate  strings
  lcd.setCursor(Lammo_pos[0]+1, Lammo_pos[1]);    
  lcd.print(Lammo_text);

  Serial.println(hp_text);
  Serial.println(Lammo_text);
}


