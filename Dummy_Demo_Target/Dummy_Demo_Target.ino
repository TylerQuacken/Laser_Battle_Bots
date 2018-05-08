/*
 * Laser Battle Bot - Tank v1.0
 * 
 * by Tyler Quackenbush & Landon Willey
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
#define shooter_led 4
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
#define buzzer 10

int Lspeed = 0;
int Rspeed = 0;
int HPmax = 500;
int HP = 500;
bool stat[] = {0,0,0};    //{slow, disable, freeze}
int turret_pos = 90;
byte sweep_dir = 1;
boolean shooting = 0;
volatile boolean IR_receiving = false;
volatile boolean IR_message_ready = false;
volatile unsigned long IR_message[] = {0, 0, 0, 0, 0, 0, 0, 0};
byte shot_config = 0b01010101;

int damage_vals[] = {0,1,3,5,10,25,50,100};   //IR transmits 0bXXX, the index to this array

Servo turret;


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

  shooting = 1;
  
  turret.attach(servo_pin);
  attachInterrupt(digitalPinToInterrupt(IR_in_pin), data_in, RISING);
  digitalWrite(buzzer,LOW); // Initialize buzzer to off state
}

void loop() {

  //Set parameters
  //turret.write(turret_pos);
  
//  if(random(0, 10) == 0){
//    shoot(shot_config);    //Fire one damage, solo mode
//  }

  
  if((turret_pos % 30) == 0){
    shoot(shot_config);    //Fire one damage, solo mode
  }

  //Get IR input
  //IR_in();

  //Receive any incoming RF transmissions



//  if(shooting == false)
    delay(20);
    turret_pos += sweep_dir;

    if(turret_pos >= 180)
    sweep_dir = -1;

    if(turret_pos <= 0)
    sweep_dir = 1;


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
  digitalWrite(buzzer, HIGH);
  digitalWrite(shooter_led, HIGH);
  delayMicroseconds(1000);
  digitalWrite(shooter_pin, LOW);
  digitalWrite(buzzer, LOW);
  digitalWrite(shooter_led, LOW);
  delayMicroseconds(1000);

  for (int i = 0; i < 8; i++){
    digitalWrite(shooter_pin, HIGH);
    digitalWrite(buzzer, HIGH);
    digitalWrite(shooter_led, HIGH);
    delayMicroseconds(transmit[i]);
    digitalWrite(shooter_pin, LOW);
    digitalWrite(buzzer, LOW);
    digitalWrite(shooter_led, LOW);
    delayMicroseconds(500);
  }

  shooting = 0;
  
}

void IR_in(){
  if (IR_receiving == true){    //If interrupt has been triggered and IR is inbound
    IR_message_ready = true;
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
      digitalWrite(buzzer, LOW);
      
    }
    
    clear_IR_message();
  }
}

void data_in(){
    IR_receiving = true;
    digitalWrite(buzzer, HIGH);
}

void clear_IR_message(){
  for (int i = 0; i < IR_message_length; i++){
    IR_message[i] = 0;
  }
  
  IR_message_ready = false;
}
