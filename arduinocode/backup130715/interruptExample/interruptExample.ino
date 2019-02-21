// Enable more interrupts than are usually accessible by arduino
// See https://github.com/GreyGnome/EnableInterrupt and the README.md for more information.
#include <EnableInterrupt.h>

// define the rotary encoder pins. These need to be interrupts for speed.
#define E1A A8
#define E1B A9
#define E2A A10
#define E2B A11
#define E3A A12
#define E3B A13
#define E4A A14
#define E4B A15
#define E5A 10
#define E5B 11


// Variables modified inside an interrupt function must be volatile to ensure they are properly updated.
volatile uint16_t E1_count=0; // The count will go back to 0 after hitting 65535.
uint16_t E1_oldcount=0;

volatile bool E1A_state=false;
volatile bool E1B_state=false;
volatile bool E1_turning=false;

void interruptE1() {
  E1A_state = digitalRead(E1A);
  E1B_state = digitalRead(E1B);
  if (E1A_state && E1B_state){ // if both pins are 1, rotation is finished so reset turn signal
    E1_turning=false;
    return;
  }
  if (E1A_state==0 && E1B_state==0){ // the encoder has reached the halfway point meaning we are turning
    E1_turning=true;
    return;
  }
  if (E1_turning){
    if(E1A_state) {E1_count++;}
    else {E1_count--;}
  }
}

// Attach the interrupt in setup()
void setup() {

  Serial.begin(115200);
  Serial.println("---------------------------------------");
  pinMode(E1A, INPUT_PULLUP);  // Configure the pin as an input, and turn on the pullup resistor.
  pinMode(E1B, INPUT_PULLUP);
  enableInterrupt(E1A, interruptE1, CHANGE);
  enableInterrupt(E1B, interruptE1, CHANGE);
}

// In the loop, we just check to see where the interrupt count is at. The value gets updated by the
// interrupt routine.
void loop() {
  delay(5);
  if (E1_count != E1_oldcount){
    Serial.println(E1_count, DEC);      // print the interrupt count.
  }
  E1_oldcount=E1_count;
}
