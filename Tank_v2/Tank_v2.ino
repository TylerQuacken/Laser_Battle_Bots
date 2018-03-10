/*
 * Laser Battle Bot - Tank v1.0
 * 
 * by Tyler Quackenbush
 * 2/3/2018
 * 
 * Library: TMRh20/RF24
 * 
 * PINS:
 * 2 IR in
 * 3~ Turret Servo DAT
 * 4  
 * 5~ motor ENA
 * 6~ motor ENB
 * 7 CE
 * 8 CS
 * 9(~) 
 * 10(~)  
 * 11~  MOSI
 * 12  MISO
 * 13  SCK
 * 
 * A1 Shooter
 * A2 IN2
 * A3 motor IN2
 * A4 IN3
 * A5 IN4
 * 
 */

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>

#define IN1 A2    //forward Left
#define IN2 A3
int ENA = 5;
#define IN3 A4    //forward Right
#define IN4 A5
int ENB = 6;
int servo_pin = 3;
#define shooter_pin A1
#define IR_in_pin 2

int cooldown = 0;     //# of millis between shots
unsigned long last_shot = 0;

#define CE_pin 7    //TX/RX mode pin
#define CSN_pin 8   //Slave select
#define input_min 0
#define input_max 255
#define inbound_len 3     //# of bytes in incoming message
#define outbound_len 3
#define IR_message_length 8

int Lspeed = 0;
int Rspeed = 0;
int HPmax = 100;
int HP = 100;
int turret_pos = 90;
boolean shooting = 0;
volatile boolean IR_receiving = false;
volatile boolean IR_message_ready = false;
//volatile unsigned long last_change = 0;
//volatile byte index = 0;
volatile unsigned long IR_message[] = {0, 0, 0, 0, 0, 0, 0, 0};

Servo turret;

RF24 radio(CE_pin, CSN_pin);
const byte controller_address[6] = "00001";
const byte tank_address[6] = "00002";

void setup() {
  // put your setup code here, to run once:
  pinMode(IN1, OUTPUT);   //set all H-bridge pins and enables to OUTPUT
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(shooter_pin, OUTPUT);
  pinMode(IR_in_pin, INPUT);
  Serial.begin(115200);
  radio.begin();
  radio.openReadingPipe(1, controller_address);
  radio.openWritingPipe(tank_address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  turret.attach(servo_pin);
  attachInterrupt(digitalPinToInterrupt(IR_in_pin), data_in, RISING);
}

void loop() {

  //Set parameters
  turret.write(turret_pos);
  drive(Lspeed, Rspeed);
  //digitalWrite(shooter_pin,shooting);
  if(shooting == true && (millis() - last_shot >= cooldown)){
    last_shot = millis();
    shoot(0b01001001);    //Fire one damage, solo mode
  }

  //Get IR input
  if (IR_receiving == true){    //If interrupt has been triggered and IR is inbound
    IR_message_ready = true;
    unsigned long pulse = 0;
    for (int i = 0; i < IR_message_length; i++){
      pulse = pulseInLong(IR_in_pin, HIGH, 50000);
      //Serial.println(pulse);
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
    for (int i = 0; i < IR_message_length; i++){
      Serial.print(IR_message[i]);
      Serial.print("    ");
    }
    Serial.println(" ");
    clear_IR_message();
  }



  //Receive any incoming transmissions

  check_inbox();


//  if(shooting == false)
//    delay(10);


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
  if(radio.available()) {   //if there's a message waiting
    
    //{Lmotor_speed, Rmotor_speed, Servo position, ...}
    byte message[inbound_len];        //stuff incoming data into 'message'
    radio.read(&message, inbound_len);
//    Serial.print(message[0]);
//    Serial.print("    ");
//    Serial.print(message[1]);
//    Serial.print("    ");
//    Serial.print(message[2]);
//    Serial.println("    ");

    Lspeed = map(message[0], 0, 255, -255, 255);    //stuff incoming data into the right spots
    Rspeed = map(message[1], 0, 255, -255, 255);
    if (bitRead(message[2],0) == 1 && turret_pos < 180) // rightmost bit is servo incrementer
      turret_pos += 3;
    if (bitRead(message[2],1) == 1 && turret_pos > 0) // second bit is servo decrementer
      turret_pos -= 3;
    shooting = (bitRead(message[2],2)); // third bit is LED activator
    
    Serial.print(Lspeed);
    Serial.print("    ");
    Serial.print(Rspeed);
    Serial.println("    ");


//    radio.stopListening();    //change to TX mode
//
//    //{hpMSbyte, hpLSbyte, 0b[~,~,~,~,~,slow,disable,freeze]}
//    byte outbound[outbound_len];    //to send to controller
//    int HPmsb = HP >> 8;
//    byte HPlsb = byte(HP);
//    outbound[0] = HPmsb;
//    outbound[1] = HPlsb;
//    outbound[2] = 0;    //will contain status effects
//
//    radio.write(&outbound, outbound_len);   //send the message, wait for acknowledge
//    radio.startListening();   //change back to RX mode
    
    
  }
}

int deadZone(int num, int radius, int deadVal){   //takes any number (usually a joystick-related value) and normalizes it 
                                                  //to a given value, it basically makes a zone of zero sensitivity in the joystick

  if(abs(num - deadVal) <= radius)
    num = deadVal;

  return num;

}

void shoot(byte message){
  
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

//  digitalWrite(out_pin, HIGH);
//  delayMicroseconds(3000);
//  digitalWrite(out_pin, LOW);
//  delayMicroseconds(6000);
  
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
}

void data_in(){
    IR_receiving = true;
    //Serial.println("flag");
  
//  IR_message[index] = micros();
//  index ++;
  
//    unsigned long change = micros();
//  if (IR_receiving == false && 7000 > change - last_change > 5000){
//    IR_receiving = true;
//  }
//  else if (IR_receiving == true && input_state == LOW){
//    input_state = HIGH;
//  }
//  else if (IR_receiving == true && input_state == HIGH){
//    input_state = LOW;
//    if (1500 < change - last_change < 2500)
//      IR_message[bit_no] = 1;
//    bit_no--;
//  }
//
//  if (bit_no < 0){
//    IR_receiving = false; 
//    bit_no = 7;
//  }
//  
//  
//  last_change = change;
  
}

void clear_IR_message(){
  for (int i = 0; i < IR_message_length; i++){
    IR_message[i] = 0;
  }
  
  IR_message_ready = false;
}
