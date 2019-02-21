#include <SimpleTimer.h>

// See https://github.com/GreyGnome/EnableInterrupt and the README.md for more information.
#include <EnableInterrupt.h> //for the encoders. Enable more interrupts than are usually accessible by arduino 
#include <Enc.h> // Matthew Harvey's simplistic encoder class
#include <MPS.h> // Matthew Harvey's simplistic MPS file. Saves space/readability in this main file
#include <Wire.h>  // used by LiquidCrystal_I2C
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
uint8_t coordX[] = {0, 6, 12, 0, 6, 12}; //places to put the numbers in line with the labels on front panel
uint8_t coordY[] = {0, 0, 0, 1, 1, 1};

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
void disableInterrupts();
void enableInterrupts();

SimpleTimer shortInterval(50);
SimpleTimer longInterval(5000);
SimpleTimer readTimer(30);

double *pVoltage; //can't return an array but i can return a pointer to the array.

void setup() {
  Serial.begin(115200); // to communicate with the pc
  Serial1.begin(9600); // to address the MAX485 used to communicate with the power MPS5 power supply modules.
  //delay(100);
  pinMode(ENABLE485, OUTPUT);
  Serial.println("Serial connected");

  lcd.begin(16,2);   // initialize the lcd for 16 chars 2 lines, turn on backlight
  
  pinMode(FP,INPUT_PULLUP);
  pinMode(BOTH,INPUT_PULLUP);
  pinMode(PC,INPUT_PULLUP);
  pinMode(TENV,INPUT_PULLUP);
  pinMode(ONEV,INPUT_PULLUP);
  pinMode(PONEV,INPUT_PULLUP);


  readTimer.reset();
  while(!readTimer.timedOut()){//flush out any spurious bytes from the serial buffers
    if(Serial1.available()){
      Serial1.read();
    }
    if(Serial.available()){
      Serial.read();
    }
  }
  
  pVoltage=getSetVoltages(); //populate the voltage array with the set values from the modules. Do this in case arduino is reset.
  for(int i=0;i<5;i++){
    Voltages[i]=*(pVoltage+i);
  }
  getActualVoltages();
  getCurrents();
  enableInterrupts(); // Enable interrupts for the rotary encoder controls
  
}



char* messageP;
void loop() {
  if(shortInterval.timedOut()) {
    // the following 4 commands handle all front panel based control
    getControlMode();                   //set fp control mode option
    getDialSetting();                   //set fp voltage option to 0.1, 1 or 10V per encoder step
    fpUpdateVoltages();                 //updates the voltage array if encoders have changed and sends new voltages to modules
    displayVoltages();                  //updates the fp LCD
    
    // the following command handle comms to and from the pc.
    messageP = readSerial(controlMode);
    if(messageP){ //carry out commands from PC if any were received.
      parseMessage(messageP, Voltages);
    }
  }
  if(longInterval.timedOut()){
    //getSetVoltages();
    //getActualVoltages();
    //getCurrents();
  }

}


////////////////////////////////////////////////////////////////////////
// Helper functions


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
      disableInterrupts();
    }  
    else{
      enableInterrupts();
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
      updateMPS(i+1,Voltages[i]); //module addresses are 1 to 5
    }
  }
}

void disableInterrupts(){
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
void enableInterrupts(){
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


