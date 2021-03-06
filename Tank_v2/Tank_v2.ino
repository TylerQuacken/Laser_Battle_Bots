/*
 * Laser Battle Bot - Tank v1.0
 * 
 * by Tyler Quackenbush & Landon Willey
 * 2/3/2018
 * 
 * Library: TMRh20/RF24
 * 
 * PINS:
 * 
 */

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>

//PIN DEFINITIONS
#define IN1 A2    //forward Left
#define IN2 A3
int ENA = 5;
#define IN3 A4    //forward Right
#define IN4 A5
int ENB = 6;
int servo_pin = 3;
#define shooter_pin A1
#define shooter_led 4
#define IR_in_pin 2
#define CE_pin 7    //TX/RX mode pin
#define CSN_pin 8   //Slave select
#define buzzer 10

int cooldown = 0;     //# of millis between shots
unsigned long last_shot = 0;

#define inbound_len 3     //# of bytes in incoming message
#define outbound_len 3
#define IR_message_length 8

int Lspeed = 0;
int Rspeed = 0;
int HPmax = 500;
int HP = 500;
bool stat[] = {0,0,0};    //{slow, disable, freeze}
int turret_pos = 90;
boolean shooting = 0;
volatile boolean IR_receiving = false;
volatile boolean IR_message_ready = false;
volatile unsigned long IR_message[] = {0, 0, 0, 0, 0, 0, 0, 0};
byte shot_config = 0;
unsigned long buzz_off = 0;

int damage_vals[] = {0,1,3,5,10,25,50,100};   //IR transmits 0bXXX, the index to this array

Servo turret;

bool listening = true;

RF24 radio(CE_pin, CSN_pin);
const byte in_address[6] = "00001";
const byte out_address[6] = "00002";

void setup() {
  // put your setup code here, to run once:
  pinMode(IN1, OUTPUT);   //set all H-bridge pins and enables to OUTPUT
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(shooter_led, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(shooter_pin, OUTPUT);
  pinMode(IR_in_pin, INPUT);
  Serial.begin(9600);
  
  radio.begin();
  radio.openReadingPipe(1, in_address);
  radio.openWritingPipe(out_address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  
  turret.attach(servo_pin);
  attachInterrupt(digitalPinToInterrupt(IR_in_pin), data_in, RISING);
  digitalWrite(buzzer,LOW); // Initialize buzzer to off state
}

void loop() {

  //ASAP parameter setting
  turret.write(turret_pos);
  drive(Lspeed, Rspeed);
  if (buzz_off > millis())
    digitalWrite(buzzer, LOW);
  if(shooting == true && (millis() - last_shot >= cooldown)){
    last_shot = millis();
    shoot(shot_config);    //Fire one damage, solo mode
  }

  //Get IR input
  IR_in();

  //Receive any incoming RF transmissions
  if(radio.available() && listening == true){
    check_inbox();
  }

  //reply to RF
  replyRF();

}

///////////////////////////////////////////////////////////////////////

void drive(int Lval, int Rval){     //Drives the motors with speed and direction [0 255]
  analogWrite(ENA, abs(Lval));
  analogWrite(ENB, abs(Rval));

//  Serial.print(abs(Lval));
//  Serial.print("    ");
//  Serial.println(Rval);

  if(Lval > 0){           //Set left direction
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  }
  else  {    
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  }
  
  if(Rval > 0){           //set right direction
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  }
  else  {    
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  }
}

void check_inbox(){
  ////////GET INCOMING////////
  //{Lmotor_speed, Rmotor_speed, Servo position, shot_configuration}
  byte message[inbound_len];        //stuff incoming data into 'message'
  radio.read(&message, inbound_len);

  radio.stopListening();    //change to TX mode
  listening = false;

    ////////ACT ON RECEIVED DATA////////
  
//    Serial.print(message[0]);
//    Serial.print("    ");
//    Serial.print(message[1]);
//    Serial.print("    ");
//    Serial.print(message[2]);
//    Serial.println("    ");

  Lspeed = map(message[0], 0, 255, -255, 255);    //stuff incoming data into the right spots
  Rspeed = map(message[1], 0, 255, -255, 255);
  if (bitRead(message[2],0) == 1 && turret_pos > 0) // rightmost bit is servo incrementer
    turret_pos -= 3;
  if (bitRead(message[2],1) == 1 && turret_pos < 180) // second bit is servo decrementer
    turret_pos += 3;
  shooting = (bitRead(message[2],2)); // third bit is LED activator
  shot_config = message[3];
  
  //Serial.print(Lspeed);
  //Serial.print("    ");
  //Serial.print(Rspeed);
  //Serial.println("    ");
}

void replyRF(){
    //{hpMSbyte, hpLSbyte, 0b[~,~,~,~,~,slow,disable,freeze]}
  byte outbound[outbound_len];    //to send to controller
  int HPmsb = HP >> 8;
  byte HPlsb = byte(HP);
  Serial.print(HP, BIN);
  Serial.print("\t");
  Serial.print(HPmsb, BIN);
  Serial.print("\t");
  Serial.print(HPlsb, BIN);
  Serial.println("\t");
  outbound[0] = HPmsb;
  outbound[1] = HPlsb;
  outbound[2] = 0;    //will contain status effects

  radio.write(&outbound, outbound_len);   //send the message, wait for acknowledge
  radio.startListening();   //change back to RX mode
  listening = true;
}

int deadZone(int num, int radius, int deadVal){   //takes any number (usually a joystick-related value) and normalizes it 
                                                  //to a given value, it basically makes a zone of zero sensitivity in the joystick

  if(abs(num - deadVal) <= radius)
    num = deadVal;

  return num;

}

void shoot(byte message){
  digitalWrite(buzzer, HIGH);
  digitalWrite(shooter_led, HIGH);
  
  int transmit[8];
  transmit[0] = 380 + 500*(boolean(0x80&message));  //team
  transmit[1] = 380 + 500*(boolean(0x40&message));  //team
  transmit[2] = 380 + 500*(boolean(0x20&message));  //dmg
  transmit[3] = 380 + 500*(boolean(0x10&message));  //dmg
  transmit[4] = 380 + 500*(boolean(0x08&message));  //dmg
  transmit[5] = 380 + 500*(boolean(0x04&message));  //slow status
  transmit[6] = 380 + 500*(boolean(0x02&message));  //cannon disable
  transmit[7] = 380 + 500*(boolean(0x01&message));  //freeze

//  Serial.print(transmit[0]);
//  Serial.print("    ");
//  Serial.print(transmit[1]);
//  Serial.print("    ");
//  Serial.print(transmit[2]);
//  Serial.print("    ");
//  Serial.print(transmit[3]);
//  Serial.print("    ");
//  Serial.print(transmit[4]);
//  Serial.print("    ");
//  Serial.print(transmit[5]);
//  Serial.print("    ");
//  Serial.print(transmit[6]);
//  Serial.print("    ");
//  Serial.print(transmit[7]);
//  Serial.println("    ");
  
  digitalWrite(shooter_pin, HIGH);
  delayMicroseconds(1000);
  digitalWrite(shooter_pin, LOW);
  delayMicroseconds(1000);

  for (int i = 0; i < 8; i++){
    digitalWrite(shooter_pin, HIGH);
    delayMicroseconds(transmit[i]);
    digitalWrite(shooter_pin, LOW);
    delayMicroseconds(500);
  }

  shooting = 0;
  
  digitalWrite(buzzer, LOW);
  digitalWrite(shooter_led, LOW);
}

void IR_in(){
  if (IR_receiving == true){    //If interrupt has been triggered and IR is inbound
    IR_message_ready = true;
    digitalWrite(buzzer, HIGH);
    buzz_off = millis() + 100;
    unsigned long pulse = 0;
    for (int i = 0; i < IR_message_length; i++){
      pulse = pulseInLong(IR_in_pin, HIGH, 50000);
      if (pulse > 750){
        IR_message[i] = 1;
      }
      else if (pulse == 0){
        clear_IR_message();
        break;
      }
    }
    IR_receiving = false;
  }
  
  if(IR_message_ready == true){
//    for (int i = 0; i < IR_message_length; i++){
//      Serial.print(IR_message[i]);
//      Serial.print("\t");
//    }
//    Serial.println(" ");

    int damage_index = IR_message[5]<<2 + IR_message[4]<<1 + IR_message[3];
    int damage_taken = damage_vals[damage_index];
    if (damage_taken > 0){
      HP -= damage_taken;
      // set buzzer high
      
    }
    
    clear_IR_message();
  }
}

void data_in(){
    IR_receiving = true;
}

void clear_IR_message(){
  for (int i = 0; i < IR_message_length; i++){
    IR_message[i] = 0;
  }
  
  IR_message_ready = false;
}
