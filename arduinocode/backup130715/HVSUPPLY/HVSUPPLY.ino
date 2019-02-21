
//#define DEBUG
// Enable more interrupts than are usually accessible by arduino
// See https://github.com/GreyGnome/EnableInterrupt and the README.md for more information.
#include <EnableInterrupt.h> //for the encoders
#include <Enc.h> // Matthew Harvey's simplistic encoder class
#include <Wire.h>  // used by LiquidCrystal_I2C
#include <LiquidCrystal_I2C.h>


LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
uint8_t coordX[] = {0, 6, 12, 0, 6, 12}; //places to put the numbers in line with the labels on front panel
uint8_t coordY[] = {0, 0, 0, 1, 1, 1};

#define STX 0x02 //start bit for RS485 comms with MPS5's
#define LF 0x0A // line feed for ^
#define ENABLE485 2 //enable read write MAX485 which is in half duplex mode

// Control mode switch
#define BOTH 40
#define PC 41
#define FP 42

// Dial setting switch
#define TENV 36
#define ONEV 35
#define PONEV 34

// ROTARY ENCODER SECTION
#define E1A 11 // define the rotary encoder pins. These need to be interrupts for speed.
#define E1B 10
#define E2A A8
#define E2B A9
#define E3A A14
#define E3B A15
#define E4A A12
#define E4B A13
#define E5A A10
#define E5B A11

//setup the five encoders using my Enc class. Just pass it the pins to use
Enc encoder[5]={{E1A, E1B},
              {E2A, E2B},
              {E3A, E3B},
              {E4A, E4B},
              {E5A, E5B}};
int controlMode[2]={0,0}; //0 for Front panel, 1 for Both, 2 for PC, second element is the previous state
double dialSetting=0;
double Voltages[5] = {}; // define voltages for the power supplies

// used to run functions at specific update intervals
unsigned long previousShortMillis = 0;
unsigned long previousLongMillis = 0;
unsigned long currentMillis = 0;
int shortInterval = 50; // how many milliseconds to wait before reading the voltages and updating the front panel display
int longInterval = 1000;

#define LEN 16 //length of message buffer
char messageOut[LEN]; //buffer for messages to MPS modules
char messageIn[LEN]; // buffer for messages from MSPS modules
char in; //read in a character from serial
int timeout = 10; // 100ms is max time we'll wait for a complete message.

//helper functions
void enc1(){encoder[0].interruptEnc();} //these wrappers are needed because I can't attach a class function to the interrupts below.
void enc2(){encoder[1].interruptEnc();} //maybe the EnableInterrupt library is c, not c++
void enc3(){encoder[2].interruptEnc();}
void enc4(){encoder[3].interruptEnc();}
void enc5(){encoder[4].interruptEnc();}
  
void displayVoltages(); // updates the display with voltages
void fpUpdateVoltages(); // update the voltages to send to the power supplies
void getControlMode(); // returns 0 for Front panel, 1 for Both, 2 for PC
void getDialSetting(); // returns 10 for 10V, 1 for 1V and 0.1 for 0.1V
void computerReceive(); // receive messages from the pc
void readSerial(); // handle serial and
void readSerial1(); // handle serial and
void updateMPS(int module_no, double voltage);
void reportVoltages();
void reportCurrents();
char calcChecksum(char* message, int len); // calculate and return the checksum
void findAddress();
  


void setup() {
  Serial.begin(9600); // to communicate with the pc
  Serial1.begin(9600); // to address the MAX485 used to communicate with the power MPS5 power supply modules.
  pinMode(ENABLE485, OUTPUT);
  Serial.println("Serial connected");
  
  // Enable interrupts for the rotary encoder controls
  enableInterrupt(E1A, enc1, CHANGE);
  enableInterrupt(E1B, enc1, CHANGE);
  enableInterrupt(E2A, enc2, CHANGE);
  enableInterrupt(E2B, enc2, CHANGE);
  enableInterrupt(E3A, enc3, CHANGE);
  enableInterrupt(E3B, enc3, CHANGE);
  enableInterrupt(E4A, enc4, CHANGE);
  enableInterrupt(E4B, enc4, CHANGE);
  enableInterrupt(E5A, enc5, CHANGE);
  enableInterrupt(E5B, enc5, CHANGE);
  
  lcd.begin(16,2);   // initialize the lcd for 16 chars 2 lines, turn on backlight
  
  pinMode(FP,INPUT_PULLUP);
  pinMode(BOTH,INPUT_PULLUP);
  pinMode(PC,INPUT_PULLUP);
  pinMode(TENV,INPUT_PULLUP);
  pinMode(ONEV,INPUT_PULLUP);
  pinMode(PONEV,INPUT_PULLUP);

  // each time the arduino reboots, it should query the set voltage of the 5 supplies and fix them to the voltage value.
  //Serial.println("Reboot");
  
}


void loop() {
  
  currentMillis = millis();
  if(currentMillis - previousShortMillis >= shortInterval) {
    previousShortMillis=currentMillis;
    getControlMode();
    getDialSetting();
    fpUpdateVoltages();
    displayVoltages();
    readSerial();
    readSerial1();
  }
  if(currentMillis - previousLongMillis >= longInterval) {
    previousLongMillis=currentMillis;
    //reportVoltages();
    //reportCurrents();
    //findAddress();
  }
  
}


////////////////////////////////////////////////////////////////////////
// Helper functions
void findAddress(){
  digitalWrite(ENABLE485, HIGH);
  delay(10);
  int len = 8;
  char message[len];
  //for(int i=0;i<5;i++){
    message[0]=STX; //start character
    message[1]='0'; //address of power supply module
    message[2]='5'; //type of power supply. 5 for MPS5
    message[3]='I';
    message[4]='D';
    message[5]='?';
    message[6]=calcChecksum(message, len);//Calculate checksum based on MPS manual
    message[7]=LF;
    for(int i=0;i<len;i++){ //report back to pc the new voltage
      Serial.print(message[i]); 
      Serial1.print(message[i]);
    }
  //}
  digitalWrite(ENABLE485, LOW);
}

char calcChecksum(char* rawMessage, int len){
  word checksum=0;
  checksum+=rawMessage[1];//add address
  checksum+=rawMessage[2];//add dev type
  for(int i=3;i<len-2;i++){ //add the command
    checksum+=rawMessage[i];
  }
  //Calculate checksum based on MPS manual
  checksum = ~checksum+1; //the checksum is currently a unsigned 16bit int. Invert all the bits and add 1.
  checksum = 0x7F & checksum; // discard the 8 MSBs and clear the remaining MSB (B0000000001111111)
  checksum = 0x40 | checksum; //bitwise or bit6 with 0x40 (or with a seventh bit which is set to 1.) (B0000000001000000)
  return (char) checksum;
}

void getDialSetting(){
  if (digitalRead(TENV) == false){
    //Serial.print("10V");
    dialSetting=10;
  }
  else if (digitalRead(ONEV) == false){
    //Serial.print("1V");
    dialSetting=1;
  }
  else{
    //Serial.print("0.1V");
    dialSetting=0.1;
  }
}

void getControlMode(){
  if (digitalRead(FP) == false){
    if (controlMode[0]!=0){ //check whether the mode has changed
      controlMode[0]=0;
    }
  }
  else if (digitalRead(BOTH) == false){
    controlMode[0]=1;
  }
  else{
    controlMode[0]=2;
  }
  //test whether this is a new state
  if(controlMode[0]!=controlMode[1]){
    controlMode[1]=controlMode[0]; //update old state to new state
    if (controlMode[0]==2){ //switch off the interrupts so that FP doesn't update voltages
      disableInterrupt(E1A);
      disableInterrupt(E1B);
      disableInterrupt(E2A);
      disableInterrupt(E2B);
      disableInterrupt(E3A);
      disableInterrupt(E3B);
      disableInterrupt(E4A);
      disableInterrupt(E4B);
      disableInterrupt(E5A);
      disableInterrupt(E5B);
    }  
    else{
      enableInterrupt(E1A, enc1, CHANGE);
      enableInterrupt(E1B, enc1, CHANGE);
      enableInterrupt(E2A, enc2, CHANGE);
      enableInterrupt(E2B, enc2, CHANGE);
      enableInterrupt(E3A, enc3, CHANGE);
      enableInterrupt(E3B, enc3, CHANGE);
      enableInterrupt(E4A, enc4, CHANGE);
      enableInterrupt(E4B, enc4, CHANGE);
      enableInterrupt(E5A, enc5, CHANGE);
      enableInterrupt(E5B, enc5, CHANGE);
    }
  }
}

void displayVoltages(){
  //lcd.clear();
  for (int i=0;i<5;i++) {
    lcd.setCursor(coordX[i], coordY[i]);
    if (Voltages[i] > 999.9){
      lcd.print(Voltages[i],0);
    }
    else if (Voltages[i] > 99.9) {
      if (i==0 || i==3){
        lcd.print(Voltages[i],0);
        lcd.print(" ");
      } else{
        lcd.print(" ");
        lcd.print(Voltages[i],0);
      }
    }
    else if (Voltages[i]> 9.91) { // 9.91 bodge because doubles aren't that accurate on arduino. 
      lcd.print(((double)Voltages[i]),1);
    }
    else{
      if (i==0 || i==3){
        lcd.print(((double)Voltages[i]),1);
        lcd.print(" ");
      } else{
        lcd.print(" ");
        lcd.print(((double)Voltages[i]),1);
      }
    }
  }
  
  lcd.setCursor(coordX[5], coordY[5]); // Display dial setting
  if (dialSetting>2){
    lcd.print(" 10V");
  }
  else if (dialSetting>0.2){
    lcd.print("  1V");
  }
  else{
    lcd.print("0.1V");
  }
}

void fpUpdateVoltages(){ // the encoder interrupts handle the number of steps. This function updates the voltages if the number of steps has changed.
  for(int i=0;i<5;i++){
    int steps=encoder[i].steps();
    if(steps!=0){
      Voltages[i]+=dialSetting*steps;
      if (Voltages[i]<0){ //Values must be between 0 and 5000
        Voltages[i]=0;
      } 
      else if (Voltages[i]>5000)
      {
        Voltages[i]=5000;
      }
      updateMPS(i,Voltages[i]);
    }
  }
}

void updateMPS(int module_no, double voltage){
  char charVoltage[6];
  dtostrf(voltage,6,1,charVoltage);//pads with spaces so need to change these to zeros
  for(int i =0;i<6;i++){
    if(charVoltage[i]==' '){
      charVoltage[i]='0';
    }
  }
  int len = 14;
  char message[len];
  message[0]=STX; //start character
  message[1]='0' + module_no; //address of power supply module
  message[2]='5'; //type of power supply. 5 for MPS5
  message[3]='V';
  message[4]='1';
  message[5]='=';
  for(int i=0;i<6;i++){
    message[6 + i]=charVoltage[i];
  }
  message[12]=calcChecksum(message,len);//Calculate checksum based on MPS manual
  message[13]=LF;
  for(int i=0;i<len;i++){ //report back to pc the new voltage
    Serial.print(message[i]);
    Serial1.print(message[i]); 
  }
}

void reportVoltage(int module_no){
  int len = 8;
  char message[len];
  
  message[0]=STX; //start character
  message[1]='0' + module_no; //address of power supply module
  message[2]='5'; //type of power supply. 5 for MPS5
  message[3]='V';
  message[4]='1';
  message[5]='?';
  message[6]=calcChecksum(message, len);//Calculate checksum based on MPS manual
  message[7]=LF;
  for(int i=0;i<len;i++){ //report back to pc the new voltage
    Serial.print(message[i]); 
  }
  
}

void reportVoltages(){
  int len = 8;
  char message[len];
  for(int i=0;i<5;i++){
    message[0]=STX; //start character
    message[1]='1' + i; //address of power supply module
    message[2]='5'; //type of power supply. 5 for MPS5
    message[3]='V';
    message[4]='1';
    message[5]='?';
    message[6]=calcChecksum(message, len);//Calculate checksum based on MPS manual
    message[7]=LF;
    for(int i=0;i<len;i++){ //report back to pc the new voltage
      Serial.print(message[i]); 
    }
  }
}

void reportCurrents(){
  int len = 8;
  char message[len];
  for(int i=0;i<5;i++){
    message[0]=STX; //start character
    message[1]='1'; //address of power supply module
    message[2]='0' + i; //type of power supply. 5 for MPS5
    message[3]='M';
    message[4]='1';
    message[5]='?';
    message[6]=calcChecksum(message, len);//Calculate checksum based on MPS manual
    message[7]=LF;
    for(int i=0;i<len;i++){ //report back to pc the new voltage
      Serial.print(message[i]); 
    }
  }
}


void computerReceive(){
  //Serial.println("timeout");
  // attempt talk to MPS supplies
  digitalWrite(ENABLE485, HIGH);
  char message[16];
  //set unit 1 to 20V
  //<STX>15V1=20.0X<LF>
  //where X is the checksum which I need to figure out how to calculate
  message[0]=STX; //start character
  message[1]='1'; //address of power supply module
  message[2]='4'; //type of power supply. 5 for MPS5
  message[3]='V';
  message[4]='1';
  message[5]='=';
  message[6]='3';
  message[7]='0';
  message[8]='0';
  message[9]='0';
  message[10]='.';
  message[11]='0';
  message[12]='v';
  message[13]=LF;
  //calculate checksum
  word checksum=0;
  checksum+=message[1];//add address
  checksum+=message[2];//add dev type
  for(int i=3;i<12;i++){ //add the command
    checksum+=(word)message[i];
  }
  //Calculate checksum based on MPS manual
  checksum = ~checksum+1; //the checksum is currently a unsigned 16bit int. Invert all the bits and add 1.
  checksum = 0x7F & checksum; // discard the 8 MSBs and clear the remaining MSB (B0000000001111111)
  checksum = 0x40 | checksum; //bitwise or bit6 with 0x40 (or with a seventh bit which is set to 1.) (B0000000001000000)
  //Serial.print("checksum: ");
  //Serial.println(char(checksum));
  digitalWrite(ENABLE485, LOW);
  
}

void readSerial(){
  if(controlMode[0] != 0){ //only run serial accept serial commands if control mode is not set to FP only
    while(Serial.available()){
      in=Serial.read();
      if (in==STX){ // if STX if found, we are at the start of a message. Read in all the data until either an LF is reached,
                    // a timeout occurs, or too many chars are read, implying an error must have occured in transmission.
        delay(10);
        unsigned long startSerialMillis=millis();
        unsigned long currentSerialMillis = startSerialMillis; // do this to see if we timeout
        bool success = false;
        int index=0;
        int timeinwhile=0;
        while( (in!=LF) && (index<15) && ((currentSerialMillis - startSerialMillis) <= timeout) ){
          if (Serial.available()){
            in=Serial.read();
            //Serial.print(in);
            if (in==LF){ // if line feed, then command has finished
              messageIn[index]='\0'; //make sure string char array is properly terminated with a null character
              index++;
              success=true;
              Serial.println("SUCCESS");
            } else{
              //success=false;
              messageIn[index]=in;
              index++;
              //Serial.print(in);
            }
          }
          currentSerialMillis=millis();
          timeinwhile= currentSerialMillis - startSerialMillis;
        }
       
        if (success==true){
          Serial.print("Length: ");
          Serial.println(index);
          digitalWrite(ENABLE485, HIGH);
          Serial.print(messageIn);
          Serial1.print(messageIn);
//          for(int i=0;i<index;i++){
//            Serial.print(messageIn[i]);
//            Serial1.print(messageIn[i]);
//          }
          digitalWrite(ENABLE485, LOW);
          Serial.println();
        }
        else if((currentSerialMillis - startSerialMillis) >= timeout){
          Serial.print("Timeout: ");
          Serial.println(timeinwhile);
        }
        else{
          Serial.println("Invalid length");
        }
      } 
      else{
        Serial.println("Message fragment discarded");
      }
    }
  }
  else{
    if(Serial.available()){Serial.println("Control mode: Front panel only");}
    while(Serial.available()){
      Serial.read(); // clear incoming buffer by reading and discarding serial available
    }
    
  }
}

void readSerial1(){
  if(controlMode[0] != 0){ //only run serial accept serial commands if control mode is not set to FP only
    while(Serial1.available()){
      in=Serial1.read();
      Serial.print(in);
      if (in==STX){ // if STX if found, we are at the start of a message. Read in all the data until either an LF is reached,
                    // a timeout occurs, or too many chars are read, implying an error must have occured in transmission.
        delay(10);
        unsigned long startSerialMillis=millis();
        unsigned long currentSerialMillis = startSerialMillis; // do this to see if we timeout
        bool success = false;
        int index=0;
        int timeinwhile=0;
        while( (in!=LF) && (index<15) && ((currentSerialMillis - startSerialMillis) <= timeout) ){
          if (Serial1.available()){
            in=Serial1.read();
            //Serial.print(in);
            if (in==LF){
              success=true;
              Serial.println("SUCCESS");
            } else{
              //success=false;
              messageIn[index]=in;
              index++;
              //Serial.print(in);
            }
          }
          currentSerialMillis=millis();
          timeinwhile= currentSerialMillis - startSerialMillis;
        }
       
        if (success==true){
          Serial.print("Length: ");
          Serial.println(index);
          for(int i=0;i<index;i++){
            Serial.print(messageIn[i]);
          }
          Serial.println();
        }
        else if((currentSerialMillis - startSerialMillis) >= timeout){
          Serial.print("Timeout: ");
          Serial.println(timeinwhile);
        }
        else{
          Serial.println("Invalid length");
        }
      } 
      else{
        Serial.println("Message fragment discarded");
      }
    }
  }
  else{
    if(Serial1.available()){Serial.println("Control mode: Front panel only");}
    while(Serial1.available()){
      Serial1.read(); // clear incoming buffer by reading and discarding serial available
    }
    
  }
}
