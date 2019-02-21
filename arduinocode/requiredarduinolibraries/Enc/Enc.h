#include <arduino.h>
#include <EnableInterrupt.h>

class Enc { // class to be accessed by interrupt functions
    int pinA;
    int pinB;
    volatile bool A_state; // Variables modified inside an interrupt function must be volatile to ensure they are properly updated.
    volatile bool B_state;
    volatile bool turning;
    volatile int count;
  public:
    Enc(int, int);
    void interruptEnc();// function to be called when a change is detected on one of the encoder pins
    int steps();// return the number of steps since last read and reset the count
};

Enc::Enc(int A, int B){
  //A_state=false;
  //B_state=false;
  pinA = A;
  pinB = B;
  turning=false;
  count=0;
  // setup the read pins
  pinMode(pinA, INPUT_PULLUP);  // Configure the pin as an input, and turn on the pullup resistor.
  pinMode(pinB, INPUT_PULLUP);
  //enableInterrupt(pinA, interruptEnc, CHANGE); //can't get this to work with the library.
  //enableInterrupt(pinB, interruptEnc, CHANGE); //call from the main code with a helper function instead.
  A_state = digitalRead(pinA); // init states
  B_state = digitalRead(pinB);
}

void Enc::interruptEnc(){
  A_state = digitalRead(pinA);
  B_state = digitalRead(pinB);
  if (A_state && B_state){ // if both pins are 1, rotation is finished so reset turn signal
    turning=false;
    return;
  }
  if (A_state==0 && B_state==0){ // the encoder has reached the halfway point meaning we are turning
    turning=true;
    return;
  }
  if (turning){
    if(A_state) {count++;}
    else {count--;}
  }
}

int Enc::steps(){ //return the number of steps since last read and reset the count.
  int tmpcount=count;
  count=0;
  return tmpcount;
}