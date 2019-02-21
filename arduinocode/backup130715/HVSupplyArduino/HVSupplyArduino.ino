#include <Wire.h>  // used by LiquidCrystal_I2C
#include <LiquidCrystal_I2C.h>

#define STX "0x02"


// https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

// define voltages for the power supplies
double Volt1 = 0;
double Volt2 = 0;
double Volt3 = 0;
double Volt4 = 0;
double Volt5 = 0;

int CMode = 0;    // Control mode: 0=Front panel only, 1=Both, 2=PC only
int DSetting = 0; // Voltage dial setting: 0=10V, 1=1V, 2=0.1V



void setup()   
{
  Serial.begin(9600);  // Used to type in characters
  lcd.begin(16,2);   // initialize the lcd for 16 chars 2 lines, turn on backlight

// ------- Quick 3 blinks of backlight  -------------
/*  for(int i = 0; i< 3; i++)
  {
    lcd.backlight();
    delay(250);
    lcd.noBacklight();
    delay(250);
  }*/
//  lcd.backlight(); // finish with backlight on  




  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("1111");
  lcd.print("  ");
  lcd.print("2222");
  lcd.print("  ");
  lcd.print("3333");

  lcd.setCursor(0,1);
  lcd.print("4444");
  lcd.print("  ");
  lcd.print("5555");
  lcd.print("  ");
  lcd.print("0.1V");
}


void loop() 
{
   /* // when characters arrive over the serial port...
    if (Serial.available()) {
      // wait a bit for the entire message to arrive
      delay(100);
      // clear the screen
      lcd.clear();
      // read all the available characters
      while (Serial.available() > 0) {
        // display each character to the LCD
        lcd.write(Serial.read());
      }
    }*/
    
}


