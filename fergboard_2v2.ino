/*
  pin assignments:
  D0- enbl_x
  D1- dir_y
  D2- scl
  D3- sda
  D4- stp_y
  D5- stp_x
  D6- dir_z
  D7- stp_z
  D8- enbl_c
  D9- sm1
  D10- sm0
  D11- dir_x
  D12- enbl_y
  D13- NO ASSIGNMENT
  D14- NO ASSIGNMENT
  D15- NO ASSIGNMENT
  D16- NO ASSIGNMENT
  A0- blue
  A1- green
  A2- red
  A3- 1w_out
  A4- 1w_in
  A5- enbl_z

  chip settings:
  SM0, SM1 - setting
  0, 0 - full step
  0, 1 - half step
  1, 0 - wave drive (quarter step)
  1, 1 - not allowed
*/

//libraries required
#include <Wire.h>
#include <SerialCommandFergboard.h>

//pin definitions
#define enbl_x 0
#define dir_y 1
#define stp_y 4
#define stp_x 5
#define dir_z 6
#define stp_z 7
#define enbl_c 8
#define sm1 9
#define sm0 10
#define dir_x 11
#define enbl_y 12
#define blue A0
#define green A1
#define red A2
#define ow_out A3
#define ow_in A4
#define enbl_z A5

//declare global variables
//speed is measured in steps/s

int x_speed = 500;
int y_speed = 500;
int z_speed = 500;

long x_period = 1000000/x_speed;
long y_period = 1000000/y_speed;
long z_period = 1000000/z_speed;

long x_pos = 0;
long y_pos = 0;
long z_pos = 0;

SerialCommand sCmd;

volatile bool twiFlag = false;
char *twiPointer;

void setup() {
  // set the pins correctly for their modes of operation
  pinMode(enbl_x, OUTPUT);
  pinMode(dir_y, OUTPUT);
  pinMode(stp_y, OUTPUT);
  pinMode(stp_x, OUTPUT);
  pinMode(dir_z, OUTPUT);
  pinMode(stp_z, OUTPUT);
  pinMode(enbl_c, OUTPUT);
  pinMode(sm1, OUTPUT);
  pinMode(sm0, OUTPUT);
  pinMode(dir_x, OUTPUT);
  pinMode(enbl_y, OUTPUT);
  pinMode(blue, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(ow_out, OUTPUT);
  pinMode(ow_in, INPUT);
  pinMode(enbl_z, OUTPUT);
  USBCON |= (1 << OTGPADE);

  // read and write all the pins to set them up correctly

  //set up the motors
  digitalWrite(enbl_x, HIGH);
  digitalWrite(dir_x, LOW);
  digitalWrite(stp_x, LOW);

  digitalWrite(enbl_y, HIGH);
  digitalWrite(dir_y, LOW);
  digitalWrite(stp_y, LOW);

  digitalWrite(enbl_z, HIGH);
  digitalWrite(dir_z, LOW);
  digitalWrite(stp_z, LOW);

  //set up the one wire interface
  digitalWrite(ow_out, LOW);
  digitalRead(ow_in);

  //set up the motors for half stepping
  digitalWrite(sm1, HIGH);
  digitalWrite(sm0, LOW);

  int address = oneWire();

  Serial.begin(115200);
  
  Wire.onReceive(receiveEvent); // register event
  Wire.onRequest(requestEvent);


  //set up the serial commands
  sCmd.addCommand("NET", network);
  sCmd.addCommand("LON", lightOn);
  sCmd.addCommand("LOF", lightOff);
  sCmd.addCommand("TST", test);
  sCmd.addCommand("STV", setVelocity);
  sCmd.addCommand("MOV", moveStage);
  sCmd.addCommand("JOG", jogStage); 
  sCmd.addCommand("POS", reportPosition);
  sCmd.addCommand("PTH", path); 
  sCmd.addCommand("SLP", sleep); 
  sCmd.addCommand("ENG", energise);
  sCmd.addCommand("STM", steppingMode);
  sCmd.addCommand("RSM", resetMotors);
  sCmd.addCommand("ERR", error);
  
  sCmd.addDefaultHandler(unrecognised);  // Handler for command that isn't matched

  //show that the board is operational
  longPulse(red, 1);
  longPulse(green, 1);
  longPulse(blue, 1);
  longPulse(green, address);
  //digitalWrite(green, HIGH);
  Serial.print("Ready\n");
}


void loop() {
  // this checks the serial queue for commands and executes them
  sCmd.readSerial();
  if(twiFlag){
    digitalWrite(green, HIGH);
    sCmd.addBufferMod(twiPointer);
    twiFlag = false;
    digitalWrite(green, LOW);
  }
}

void receiveEvent() {
  volatile char twiBuffer[32];
  volatile char *wireChar;
  volatile int i = 0;
  while (Wire.available()) { 
    twiBuffer[i] = Wire.read();
    i++;
  }
  twiBuffer[i] = NULL;
 
//  wireChar = Wire.read();
//  strcpy(twiBuffer,wireChar);
//
//  while( Wire.available()) {
//    wireChar = Wire.read();
//    strcat(twiBuffer,wireChar);
//  }
  
  twiPointer = twiBuffer;
  twiFlag = true;
}

void requestEvent() {
  char buf [4] = { '1', '2', '3', '4' };
  Wire.write (buf, sizeof buf);  
}

void longPulse(int pin, int num) {
  for (int i = 0; i < num; i++) {
    digitalWrite(pin, HIGH);
    delay (200);
    digitalWrite(pin, LOW);
    delay (200);
  }
}

void lightOn(){
  digitalWrite(red, HIGH);
}

void lightOff(){
  digitalWrite(red, LOW);
}

void pulse(int pin) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(2);
    digitalWrite(pin, LOW);
}

void test(){
    //char command[] = {'M', 'O', 'V', ' ', '5', '0', '0',' ', '5', '0', '0',' ', '5', '0', '0', '\r'};
   // char *string = command;
    //sCmd.addBufferMod(command);
    longPulse(blue,4);
}

void network(){
  int board;
  char *wirecommand;
  char commandstring[32];
  char *commandstringref;
  board = atoi(sCmd.next());
  wirecommand = sCmd.next();
  strcpy(commandstring, wirecommand);

  if(board == 1){
    // load into own buffer
     while(wirecommand != NULL){
      wirecommand = sCmd.next();
      strcat(commandstring, " ");
      strcat(commandstring, wirecommand);
     }
     strcat(commandstring, "\r");
     commandstringref = commandstring;
     sCmd.addBufferMod(commandstringref);  
  }
  else{
    // send over wire
    Wire.beginTransmission(board);
     while(wirecommand != NULL){
      Wire.write(wirecommand);
      Wire.write(" ");
      wirecommand = sCmd.next();
     }
     Wire.write("\r");
     Wire.endTransmission();
  }
}

void setVelocity(){

  x_speed = atoi(sCmd.next());
  y_speed = atoi(sCmd.next());
  z_speed = atoi(sCmd.next());

  x_period = 1000000/x_speed;
  y_period = 1000000/y_speed;
  z_period = 1000000/z_speed;
  
}

void moveStage(){
   //this may require alteration to deal with different stepping modes
  char *arg;
 
  long x_count = 0;
  long y_count = 0;
  long z_count = 0;
  long x_target = 0;
  long y_target = 0;
  long z_target = 0;

// read in the arguments and cast them as ints
  x_target = atoi(sCmd.next());
  y_target = atoi(sCmd.next());
  z_target = atoi(sCmd.next());

  x_count = x_target - x_pos;
  y_count = y_target - y_pos;
  z_count = z_target - z_pos;

  x_pos = x_target;
  y_pos = y_target;
  z_pos = z_target;

  
// set the direction lines according to the polarity of the arguments
  if (x_count < 0){
    x_count = abs(x_count);
    digitalWrite(dir_x,HIGH);
  }
  else{
    digitalWrite(dir_x,LOW);
  }

  if (y_count < 0){
    y_count = abs(y_count);
    digitalWrite(dir_y,HIGH);
  }
    else{
    digitalWrite(dir_y,LOW);
  }

  if (z_count < 0){
    z_count = abs(z_count);
    digitalWrite(dir_z,HIGH);
  }
    else{
    digitalWrite(dir_z,LOW);
  }


digitalWrite(enbl_x, LOW);
digitalWrite(enbl_y, LOW);
digitalWrite(enbl_z, LOW);

long xtest = micros();
long ytest = micros();
long ztest = micros();

long xtestadj = 0;
long ytestadj = 0;
long ztestadj = 0;

long donetime = micros();

long x_timer = micros();
long y_timer = micros();
long z_timer = micros();

long xerror = 0;
long yerror = 0;
long zerror = 0;



while(x_count > 0 || y_count > 0 || z_count > 0){

  if(x_count > 0){
    xtest = (micros() - x_timer);
    if(xtest > x_period - xtestadj){
        x_timer = micros();
        xerror = xerror + (xtest - x_period);
        xtestadj = xtestadj + (xtest - x_period) + 6;
        pulse(stp_x);
        x_count -- ;
    }
  }

  if(y_count > 0){
   ytest = (micros() - y_timer);
    if(ytest > y_period - ytestadj){
        y_timer = micros();
        yerror = yerror + (ytest - y_period);
        ytestadj = ytestadj + (ytest - y_period) + 6;
        pulse(stp_y);
        y_count -- ;
    }
  }

  if(z_count > 0){
    ztest = (micros() - z_timer);
    if(ztest > z_period - ztestadj){
        z_timer = micros();
        zerror = zerror + (ztest - z_period);
        ztestadj = ztestadj + (ztest - z_period) + 6;
        pulse(stp_z);
        z_count -- ;
    }
  }

}

donetime = micros() - donetime;

digitalWrite(enbl_x, HIGH);
digitalWrite(enbl_y, HIGH);
digitalWrite(enbl_z, HIGH);

Serial.print("Done Moving in ");

float floattime = donetime/1000000.0;

Serial.print(floattime,6);

Serial.print(" seconds\n");

}

void jogStage(){
  digitalWrite(green, LOW);
  digitalWrite(blue, HIGH);
  //this may require alteration to deal with different stepping modes
  char *arg;
  int x_flag = 0;
  int y_flag = 0;
  int z_flag = 0;
  long x_count = 50;
  long y_count = 50;
  long z_count = 50;

// read in the arguments and cast them as ints
  x_flag = atoi(sCmd.next());
  y_flag = atoi(sCmd.next());
  z_flag = atoi(sCmd.next());

// set the direction lines according to the polarity of the arguments
  if (x_flag == -1){
    digitalWrite(dir_x,HIGH);
  }
  else if (x_flag == 1){
    digitalWrite(dir_x,LOW);
  }
  else{
    x_count = 0;
  }

  if (y_flag == -1){
    digitalWrite(dir_y,HIGH);
  }
  else if (y_flag == 1){
    digitalWrite(dir_y,LOW);
  }
  else{
    y_count = 0;
  }

  if (z_flag == -1){
    digitalWrite(dir_z,HIGH);
  }
  else if (z_flag == 1){
    digitalWrite(dir_z,LOW);
  }
  else{
    z_count = 0;
  }

digitalWrite(enbl_x, LOW);
digitalWrite(enbl_y, LOW);
digitalWrite(enbl_z, LOW);

long xtest = micros();
long ytest = micros();
long ztest = micros();

long xtestadj = 0;
long ytestadj = 0;
long ztestadj = 0;

long donetime = micros();

long x_timer = micros();
long y_timer = micros();
long z_timer = micros();

long xerror = 0;
long yerror = 0;
long zerror = 0;

while(x_count > 0 || y_count > 0 || z_count > 0){

  if(x_count > 0){
    xtest = (micros() - x_timer);
    if(xtest > x_period - xtestadj){
        x_timer = micros();
        xerror = xerror + (xtest - x_period);
        xtestadj = xtestadj + (xtest - x_period) + 6;
        pulse(stp_x);
        x_count -- ;
    }
  }

  if(y_count > 0){
   ytest = (micros() - y_timer);
    if(ytest > y_period - ytestadj){
        y_timer = micros();
        yerror = yerror + (ytest - y_period);
        ytestadj = ytestadj + (ytest - y_period) + 6;
        pulse(stp_y);
        y_count -- ;
    }
  }

  if(z_count > 0){
    ztest = (micros() - z_timer);
    if(ztest > z_period - ztestadj){
        z_timer = micros();
        zerror = zerror + (ztest - z_period);
        ztestadj = ztestadj + (ztest - z_period) + 6;
        pulse(stp_z);
        z_count -- ;
    }
  }

}

donetime = micros() - donetime;

digitalWrite(enbl_x, HIGH);
digitalWrite(enbl_y, HIGH);
digitalWrite(enbl_z, HIGH);

Serial.println("FIN");

digitalWrite(blue, LOW);
digitalWrite(green, HIGH);
}

void reportPosition(){
  Serial.print(x_pos);
  Serial.print(" ");
  Serial.print(y_pos);
  Serial.print(" ");
  Serial.println(z_pos);
}

void path(){
 char *arg;

   
}

void sleep(){
  char *arg;

}

void energise(){
 
digitalWrite(enbl_x, LOW);
digitalWrite(enbl_y, LOW);
digitalWrite(enbl_z, LOW);
}

void steppingMode(){
  char *arg;

}

void resetMotors(){
 char *arg;

}

void error(){
  char *arg;

}

int oneWire(){
  int address = 1;
  bool middle = false;
  bool wait = true;

  digitalWrite(enbl_c,HIGH);
              
  if(USBSTA&(1<<VBUS)){
    Wire.begin(address);
      delay(100);
    digitalWrite(ow_out, HIGH);
            }

  else{
    Wire.begin();
    digitalWrite(red,HIGH);
    delay(100);
    while(wait){
      wait = digitalRead(ow_in);
      delay(100);
                }
    digitalWrite(red,LOW);
    delay(100);
                
       address = sweeper();
       Wire.begin(address);
       digitalWrite(ow_out, HIGH);
      }
  return address;
}

int sweeper() {
  int i = 2;
  int trans = 0;
  bool check = false;
  while(!check){
  Wire.beginTransmission(i);
  trans = Wire.endTransmission();
  if(trans == 2){
    check = true;
                }
  else{
   i++;               
      }
               }
         return i;
               }

void unrecognised()
{
  Serial.println("Command Not Recognised");
}

