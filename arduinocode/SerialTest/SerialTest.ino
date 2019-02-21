
#define STX 0x02 //start bit for RS485 comms with MPS5's
#define LF 0x0A // line feed for ^

void setup() {

  Serial.begin(9600); // to communicate with the pc
  Serial.println("Serial connected");
}

void readSerial();

unsigned long previousShortMillis = 0;
unsigned long previousLongMillis = 0;
unsigned long currentMillis = 0;

int shortInterval = 50; // how many milliseconds to wait before reading the voltages and updating the front panel display
int longInterval = 1000;

//the following 4 lines are needed by readSerial()
const int len = 64; //length of message buffer
char messageIn[len]; // buffer for messages from MSP5 modules
char in;
int timeout = 10; // 10ms is max time we'll wait for a complete message.

void loop() {
  
  readSerial();
  
 
  currentMillis = millis();
  if(currentMillis - previousShortMillis >= shortInterval) {
    previousShortMillis=currentMillis;
    
  }
  if(currentMillis - previousLongMillis >= longInterval) {
    previousLongMillis=currentMillis;
    //Serial.println("timeout");    
  }
}

void readSerial(){
  while(Serial.available()){
    in=Serial.read();
    if (in==STX){ // if STX if found, we are at the start of a message. Read in all the data until either an LF is reached,
                  // a timeout occurs, or too many chars are read, implying an error must have occured in transmission.
      delay(500);
      unsigned long startSerialMillis=millis();
      unsigned long currentSerialMillis = startSerialMillis; // do this to see if we timeout
      bool success = false;
      int index=0;
      int timeinwhile=0;
      while( (in!=LF) && (index<15) && ((currentSerialMillis - startSerialMillis) <= timeout) ){
        if (Serial.available()){
          in=Serial.read();
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
      Serial.print("Message fragment discarded");
    }
  }
}


