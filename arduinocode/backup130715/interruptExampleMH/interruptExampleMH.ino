
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

// Control mode switch
#define FP 40
#define BOTH 41
#define PC 42

// Dial setting switch
#define TENV 36
#define ONEV 35
#define POINTONEV 34

// ROTARY ENCODER SECTION
#define E1A A8 // define the rotary encoder pins. These need to be interrupts for speed.
#define E1B A9
#define E2A A10
#define E2B A11
#define E3A A12
#define E3B A13
#define E4A A14
#define E4B A15
#define E5A 10
#define E5B 11

Enc encoder1(E1A, E1B); //setup the five encoders using my Enc class. Just pass it the pins to use
Enc encoder2(E2A, E2B);
Enc encoder3(E3A, E3B);
Enc encoder4(E4A, E4B);
Enc encoder5(E5A, E5B);

//helper functions
void enc1(){encoder1.interruptEnc();} //these wrappers are needed because I can't attach a class function to the interrupts below.
void enc2(){encoder2.interruptEnc();} //maybe the EnableInterrupt library is c, not c++
void enc3(){encoder3.interruptEnc();}
void enc4(){encoder4.interruptEnc();}
void enc5(){encoder5.interruptEnc();}

void displayVoltages(); // updates the display with voltages
void getControlMode(); // returns 0 for Front panel, 1 for Both, 2 for PC
void getDialSetting(); // returns 10 for 10V, 1 for 1V and 0.1 for 0.1V

void setup() {

  Serial.begin(115200);
  
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
  pinMode(POINTONEV,INPUT_PULLUP);
}

int steps1;
int steps2;
int steps3;
int steps4;
int steps5;

int controlMode=0;
double dialSetting=0;

// define voltages for the power supplies
unsigned int Voltages[5] = {}; //stores the power supply voltages divide by 10 for actual value in volts


unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
int interval = 100; // how many milliseconds to wait before reading the voltages and updating the front panel display


void loop() {

  steps1+=encoder1.steps();
  steps2+=encoder2.steps();
  steps3+=encoder3.steps();
  steps4+=encoder4.steps();
  steps5+=encoder5.steps();
  
  #ifdef DEBUG
    Serial.print(steps1, DEC);      // print the interrupt count.
    Serial.print(", ");
    Serial.print(steps2, DEC);      // print the interrupt count.
    Serial.print(", ");
    Serial.print(steps3, DEC);      // print the interrupt count.
    Serial.print(", ");
    Serial.print(steps4, DEC);      // print the interrupt count.
    Serial.print(", ");
    Serial.println(steps5, DEC);      // print the interrupt count.
    delay(200);
  #endif
  
  currentMillis = millis();
  if(currentMillis - previousMillis >= interval) {
    getControlMode();
    getDialSetting();
    displayVoltages();
  }
  
}

void getDialSetting(){
  if (digitalRead(TENV) == false){
    Serial.print("10V");
    dialSetting=10;
  }
  else if (digitalRead(ONEV) == false){
    Serial.print("1V");
    dialSetting=1;
  }
  else{
    Serial.print("0.1V");
    dialSetting=0.1;
  }
}

void getControlMode(){
  if (digitalRead(FP) == false){controlMode=0;}
  else if (digitalRead(BOTH) == false){controlMode=1;}
  else{controlMode=2;}
}

void displayVoltages(){
  //lcd.clear();
  for (int i=0;i<5;i++) {
    lcd.setCursor(coordX[i], coordY[i]);
    if (Voltages[i] > 999){
      lcd.print(Voltages[i]/10);
    }
    else if (Voltages[i] > 99) {
      lcd.print(" ");
      lcd.print(Voltages[i]/10);
    }
    else {
      lcd.print(((double)Voltages[i])/10,1);
    }
  }
  lcd.setCursor(coordX[5], coordY[5]);
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

