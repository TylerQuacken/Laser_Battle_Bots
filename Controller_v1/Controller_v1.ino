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

#define button0 3    //right button
#define button1 2    //left button
#define button2 4    //shooter button
#define button3 5   //weapon select

#define RS 9
#define EN 10
#define d4 A2
#define d5 A3
#define d6 A4
#define d7 A5

// Create an instance of the radio transmission
RF24 radio(7, 8); // numbers define CE CSN pins
const byte out_address[6] = "00001";
const byte in_address[6] = "00002";

// Declare x and y input pins
byte y_pin = 0;
byte x_pin = 1;

// Declare a variable to store all four button states
byte buttons = 0;

// LCD Display Variables
LiquidCrystal lcd(RS, EN, d4, d5, d6, d7);
byte Lgun_char = 0;    //set char numbers for custom chars (0-7)
byte heart_char = 1;   //call lcd.write(XXX_char) to display char
byte shield_char = 2;
byte ammo_char = 3;
// columns are 0 to 15, rows are 0 to 1
byte hp_pos[] = {0,0};     //col, row of position to display each vital
byte Lammo_pos[] = {10,0};
byte weapon_pos[] = {0,1};

// Player status
int hp = 500;
int hp_max = 500;
int ammo[] = {99, 10, 99, 5};
int ammo_max[] = {99, 10, 99, 5};
// Weapon variables
String weapon_types[] = {"Basic Cannon","Heavy Cannon","Machine Gun","Reload"};
int cooldown_times[] = {300,2500,50,500};
int weapon_damage[] = {10,50,6,0}; // This needs to be encoded into an additional byte with attack
byte num_weapons = 4;
byte weapon_select = 0;
unsigned long select_cooldown = 0; // weapon select cooldown
unsigned long weapon_cooldown = 0; // weapon shooter cooldown--this could be an array to make them independent
// other options include radial blast, regenative shield, machine gun, nuke (100 damage), homing missile?
// It would be nice if we had a back button... like if there were bumper switches like on a ps2 controller.

// Millis is milliseconds since the arduino turned on

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
  lcd.createChar(ammo_char, bullet_bits);
  
  printVitals();    //prints health and ammo to LCD (control_func)

  // LCD outputs
  pinMode(RS, OUTPUT);
  pinMode(EN, OUTPUT);
  pinMode(d4, OUTPUT);
  pinMode(d5, OUTPUT);
  pinMode(d6, OUTPUT);
  pinMode(d7, OUTPUT);
  // Button inputs
  pinMode(button1, INPUT_PULLUP); // Make buttons digital inputs
  pinMode(button0, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(button3, INPUT_PULLUP);
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
  int select_1 = digitalRead(button3);
  delay(10);
  
  boolean left = digitalRead(button1) && left_1;
  boolean right = digitalRead(button0) && right_1;
  boolean shooter = digitalRead(button2) && shooter_1;
  boolean select = digitalRead(button3) && select_1;

  bitWrite(buttons,0,right);
  bitWrite(buttons,1,left);
  //bitWrite(buttons,2,shooter);

  if (ammo[weapon_select] > 0)
  {
    if (!shooter && millis() > weapon_cooldown)
    {
      // This assumes reload is the last option
      if (weapon_select == num_weapons-1)
      {
        ammo[weapon_select]--;
        weapon_cooldown = millis() + cooldown_times[weapon_select];
        for(byte i = 0; i < num_weapons-1; i++)
          ammo[i] = ammo_max[i];
      }
      else
        bitWrite(buttons,2,0);
    }
    else
      bitWrite(buttons,2,1);
  }
  else
    bitWrite(buttons,2,1);

  // Put together all commands into a message
  byte message[] = {leftWheel, rightWheel, buttons};
  
  // write returns true if successful
  if (radio.write(&message, sizeof(message)) && !bitRead(buttons,2) && ammo[weapon_select] > 0)
  {
      ammo[weapon_select]--;
      weapon_cooldown = millis() + cooldown_times[weapon_select];
  }

  
  Serial.print(message[0]);
  Serial.print("\t");
  Serial.print(message[1]);
  Serial.print("\t");
  Serial.print(bitRead(message[2],0));
  Serial.print(bitRead(message[2],1));
  Serial.print(bitRead(message[2],2));
  Serial.print(select);
  Serial.println("");
    
  // Check weapon select
  if (!select && millis() > select_cooldown)
  {
    weapon_select += 1;
    if (weapon_select > num_weapons - 1)
      weapon_select = 0;
    select_cooldown = millis() + 300;
  }

  printVitals();

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
  
  lcd.setCursor(hp_pos[0], hp_pos[1]);    //display the hp icon
  lcd.write(heart_char);
  //display current health
  String hp_text;   //will contain 'XXX/XXX'
  if (hp < 100)
    hp_text += " ";   //precede with "_" if hp has fewer digits
  if (hp < 10)
    hp_text += " ";
  hp_text += String(hp) + '/' + String(hp_max);   //concatenate  strings
  lcd.setCursor(hp_pos[0]+1, hp_pos[1]);    
  lcd.print(hp_text);
  
  lcd.setCursor(Lammo_pos[0], Lammo_pos[1]);    //display the bullet icon
  lcd.write(ammo_char);
  //display current health
  String ammo_text;   //will contain 'XX/XX'
  if (ammo[weapon_select] < 10)   //precede with "_" if fewer digits
    ammo_text += " ";
  ammo_text += String(ammo[weapon_select]) + '/';
  if (ammo_max[weapon_select] < 10)   //precede with "_" if fewer digits
    ammo_text += " ";
  ammo_text += String(ammo_max[weapon_select]);   //concatenate  strings
  lcd.setCursor(Lammo_pos[0]+1, Lammo_pos[1]);    
  lcd.print(ammo_text);

  lcd.setCursor(weapon_pos[0], weapon_pos[1]);    //display the light gun icon
  lcd.write(Lgun_char);
  String weapon_string = weapon_types[weapon_select];
  while (weapon_string.length() < 13)
    weapon_string += " ";
  //display current weapon selection
  lcd.setCursor(weapon_pos[0]+1, weapon_pos[1]);    
  lcd.print(weapon_string);

//  Serial.println(hp_text);
//  Serial.println(ammo_text);
//  Serial.println(weapon_types[weapon_select]);
}


