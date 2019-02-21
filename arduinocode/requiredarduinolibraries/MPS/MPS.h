#include <arduino.h>

#define STX 0x02 //start bit for RS485 comms with MPS5's
#define LF 0x0A // line feed for ^
#define ENABLE485 2 //enable read write MAX485 which is in half duplex mode
#define LEN 16 //length of message buffer

int timeout = 100; // 100ms is max time we'll wait for a complete message.

char calcChecksum(char* message, int len); // calculate and return the checksum
void updateMPS(int module_no, double voltage); // send new voltage to an MPS module
double* getSetVoltages();
double getSetVoltage(int module_no);
double* getActualVoltages();
double getActualVoltage(int module_no);
double* getCurrents();
double getCurrent(int module_no);
//void setAddress(int old_address, int new_address); // set the address of the devices. Make sure only one is plugged in at a time!
char* readSerial(int* controlMode); // handle serial comms with pc
void parseMessage(char* messageP, double* voltages); // parse the serial message and carry out correct response.
void(* resetFunc) (void) = 0; //declare reset function @ address 0


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

void updateMPS(int module_no, double voltage){ 
  char charVoltage[7];
  dtostrf(voltage,6,1,charVoltage);//pads with spaces so need to change these to zeros
  for(int i =0;i<6;i++){
    if(charVoltage[i]==' '){
      charVoltage[i]='0';
    }
  }
  charVoltage[6]='\0';
  
	
  char message[16]; //make atleast one longer than actual message to ensure '\0' fits to end message.
  message[0]=(char)STX; //start character
  message[1]='0' + module_no; //address of power supply module
  message[2]='5'; //type of power supply. 5 for MPS5
  message[3]='V';
  message[4]='1';
  message[5]='=';
  for(int i=0;i<6;i++){
    message[6 + i]=charVoltage[i];
  }
  message[12]=calcChecksum(message,14);//Calculate checksum based on MPS manual
  message[13]=(char)LF;
	message[14]='\0'; //properly terminate the char array.
	
  digitalWrite(ENABLE485, HIGH); //make high to send.
  delay(1);
  /*for(int i=0;i<len;i++){ //send voltage set request to MPS module
    Serial.print(message[i]);
    Serial1.print(message[i]); 
  }*/
	Serial.print(message);
	Serial1.print(message);
  delay(17); // leave enough time for whole message to get sent
  digitalWrite(ENABLE485, LOW); //make low to receive again.
	
	char in='0';
	SimpleTimer t(30); // wait up to 30ms to receive the voltage response from the module
	char messageRxMPS[LEN];
	int index = 0;
	while(in!=LF && !t.timedOut()){ // keep receiving until either a timeout or a LF character
		if(Serial1.available()){
			in=Serial1.read();
			messageRxMPS[index]=in;
			index++;
		}
	}
}

double* getSetVoltages(){
	static double tmpVoltages[5]; //must be static so that the array is persistent even when leaving function. Or make it global.
  for(int i=0;i<5;i++){
		tmpVoltages[i] = -1;//do this to make sure the new value is correctly read.
    tmpVoltages[i] = getSetVoltage(i+1);

		while(tmpVoltages[i]<0.0 || tmpVoltages[i]>5000.0){
			tmpVoltages[i] = getSetVoltage(i+1);
		}
  }
	return tmpVoltages;
}

double getSetVoltage(int module_no){
  int len = 8;
  char messageTxMPS[len];
  
  messageTxMPS[0]=(char)STX; //start character
  messageTxMPS[1]='0' + module_no; //address of power supply module
  messageTxMPS[2]='5'; //type of power supply. 5 for MPS5
  messageTxMPS[3]='V';
  messageTxMPS[4]='1';
  messageTxMPS[5]='?';
  messageTxMPS[6]=calcChecksum(messageTxMPS, len);//Calculate checksum based on MPS manual
  messageTxMPS[7]=(char)LF;

  digitalWrite(ENABLE485, HIGH); //make high to send.
  delay(1);
  for(int i=0;i<len;i++){ //send voltage request to MPS module
    Serial1.print(messageTxMPS[i]); 
  }
  delay(9); // leave enough time for whole message to get sent
  digitalWrite(ENABLE485, LOW); //make low to receive again.
  
	char in='0'; 
	SimpleTimer t(30); // wait up to 30ms to receive the voltage response from the module
	len = 16;
	char messageRxMPS[len];
	int index = 0;
	while(in!=LF && !t.timedOut()){ // keep receiving until either a timeout or a LF character
		if(Serial1.available()){
			in=Serial1.read();
			messageRxMPS[index]=in;
			index++;
		}
	}
	
	if(calcChecksum(messageRxMPS, index) != messageRxMPS[index-2]){
		return -100.0; //if the checksum doesn't match, return an out of range number so the calling routine knows it went wrong.
	}
	
	char voltageChar[7]={};
	//parse received message and send result to pc 
	for(int i=0;i<5;i++){
		Serial.print(messageTxMPS[i]);
	}
	Serial.print("=");
	for(int i=3;i<index-2;i++){
		voltageChar[i-3]=messageRxMPS[i];
		Serial.print(messageRxMPS[i]);
	}
	Serial.print("V"); //bogus checksum since I don't check it. Might as well be V for volts
	Serial.print((char)LF);
	voltageChar[index-5]='\0'; //add a '\0' char so that the atof can read a properly terminated char array.
	return atof(voltageChar);
}

double* getActualVoltages(){
	double Voltages[5]={};
  for(int i=0;i<5;i++){
    Voltages[i] = getActualVoltage(i+1);
  }
	return Voltages;
}

double getActualVoltage(int module_no){
  int len = 8;
  char messageTxMPS[len];
  
  messageTxMPS[0]=(char)STX; //start character
  messageTxMPS[1]='0' + module_no; //address of power supply module
  messageTxMPS[2]='5'; //type of power supply. 5 for MPS5
  messageTxMPS[3]='M';
  messageTxMPS[4]='0';
  messageTxMPS[5]='?';
  messageTxMPS[6]=calcChecksum(messageTxMPS, len);//Calculate checksum based on MPS manual
  messageTxMPS[7]=(char)LF;

  digitalWrite(ENABLE485, HIGH); //make high to send.
  delay(1);
  for(int i=0;i<len;i++){ //send voltage request to MPS module
    Serial1.print(messageTxMPS[i]); 
  }
  delay(9); // leave enough time for whole message to get sent
  digitalWrite(ENABLE485, LOW); //make low to receive again.
  
	char in='0';
	SimpleTimer t(30); // wait up to 30ms to receive the voltage response from the module
	len = 16;
	char messageRxMPS[len];
	int index = 0;
	Serial.println(t.elapsed())
	while(in!=LF && !t.timedOut()){ // keep receiving until either a timeout or a LF character
		if(Serial1.available()){
			in=Serial1.read();
			messageRxMPS[index]=in;
			index++;
		}
	}
	
	char voltageChar[7]={};
	//parse received message and send result to pc 
	for(int i=0;i<5;i++){
		Serial.print(messageTxMPS[i]);
	}
	Serial.print("=");
	for(int i=3;i<index-2;i++){
		voltageChar[i-3]=messageRxMPS[i];
		Serial.print(messageRxMPS[i]);
	}
	Serial.print("V"); //bogus checksum since I don't check it. V for volts
	Serial.print((char)LF);
	voltageChar[index-5]='\0'; //add a '\0' char so that the atof can read a properly terminated char array.
	return atof(voltageChar);
}

double* getCurrents(){
	double currents[5]={};
  for(int i=0;i<5;i++){
    currents[i] = getCurrent(i+1);
  }
	return currents;
}

double getCurrent(int module_no){
  int len = 8;
  char messageTxMPS[len];
  
  messageTxMPS[0]=(char)STX; //start character
  messageTxMPS[1]='0' + module_no; //address of power supply module
  messageTxMPS[2]='5'; //type of power supply. 5 for MPS5
  messageTxMPS[3]='M';
  messageTxMPS[4]='1';
  messageTxMPS[5]='?';
  messageTxMPS[6]=calcChecksum(messageTxMPS, len);//Calculate checksum based on MPS manual
  messageTxMPS[7]=(char)LF;

  digitalWrite(ENABLE485, HIGH); //make high to send.
  delay(1);
  for(int i=0;i<len;i++){ //send voltage request to MPS module
    Serial1.print(messageTxMPS[i]); 
  }
  delay(9); // leave enough time for whole message to get sent
  digitalWrite(ENABLE485, LOW); //make low to receive again.
  
	char in='0';
	SimpleTimer t(30); // wait up to 30ms to receive the voltage response from the module
	len = 16;
	char messageRxMPS[len];
	int index = 0;
	while(in!=LF && !t.timedOut()){ // keep receiving until either a timeout or a LF character
		if(Serial1.available()){
			in=Serial1.read();
			messageRxMPS[index]=in;
			index++;
		}
	}
	char currentChar[7]={};
	//parse received message and send result to pc 
	for(int i=0;i<5;i++){
		Serial.print(messageTxMPS[i]);
	}
	Serial.print("=");
	for(int i=3;i<index-2;i++){
		currentChar[i-3]=messageRxMPS[i];
		Serial.print(messageRxMPS[i]);
	}
	Serial.print("u"); //bogus checksum since I don't check it. u for micro amps.
	Serial.print((char)LF);
	currentChar[index-5]='\0'; //add a '\0' char so that the atof can read a properly terminated char array.
	return atof(currentChar);
}

/*void setAddress(int old_address, int new_address){
  int len = 9;
  char messageTxMPS[len];
  Serial.println(new_address);
	char test='0'+new_address;
	Serial.println(test);
	if(new_address < 6 && new_address > 0){
		messageTxMPS[0]=(char)STX; //start character
		messageTxMPS[1]='0' + old_address; //address of power supply module
		messageTxMPS[2]='5'; //type of power supply. 5 for MPS5
		messageTxMPS[3]='I';
		messageTxMPS[4]='D';
		messageTxMPS[5]='=';
		messageTxMPS[6]='0' + new_address;
		messageTxMPS[7]=calcChecksum(messageTxMPS, len);//Calculate checksum based on MPS manual
		messageTxMPS[8]=(char)LF;

		digitalWrite(ENABLE485, HIGH); //make high to send.
		delay(1);
		for(int i=0;i<len;i++){ //send voltage request to MPS module
			Serial.print(messageTxMPS[i]); 
			Serial1.print(messageTxMPS[i]); 
		}
		delay(20); // leave enough time for whole message to get sent
		digitalWrite(ENABLE485, LOW); //make low to receive again.
		
		char in='0';
		SimpleTimer t(30); // wait up to 30ms to receive the voltage response from the module
		len = 16;
		char messageRxMPS[len];
		int index = 0;
		while(in!=LF && !t.timedOut()){ // keep receiving until either a timeout or a LF character
			if(Serial1.available()){
				in=Serial1.read();
				messageRxMPS[index]=in;
				index++;
			}
		}
	}
	else{
		Serial.println("Address must be between 1 and 5");
	}
}*/

char* readSerial(int* controlMode){
	// Read in Serial from the PC. If the controls are restricted to the front panel,
	// return a message saying so. If the message is not properly received report that. Return a null pointer to the char array.
	// If message is accepted, return a pointer to the message and the length.
	char* messagePointer = NULL;
	char in='0';
	static char messageRxPC[LEN];
	messageRxPC[LEN]={};
	int index = 0;
	if(Serial.available()){
		SimpleTimer t(30); // wait up to 30ms to receive the voltage response from the module
		while(in!=LF && !t.timedOut()){ // keep receiving until either a timeout or a LF character
			if(Serial.available()){
				in=Serial.read();
				messageRxPC[index]=in;
				index++;
			}
		}
		messageRxPC[index+1]='\0'; //properly terminate the array so that we can parse it later on.
		if(t.timedOut()==false){ // if the timer hasn't timed out, then the message was successfully received.
			if(controlMode[0] != 0){ // only do something if controls aren't restricted to front panel only.
				if(calcChecksum(messageRxPC,index)==messageRxPC[index-2]){ //check that checksum matches.
					messagePointer = messageRxPC;
				}
				else{
					Serial.print((char)STX);
					Serial.print("ChecksumError");
					Serial.print((char)LF);
				}
			} 
			else{ // if locked to FP, let PC user know.
				Serial.print((char)STX);
				Serial.print("FrontPanelOnly");
				Serial.print((char)LF);
			}
		}
	}
	return messagePointer;
}

void parseMessage(char* messageP, double* voltages){
	int module_no=*(messageP+1)-'0'; // get the module number. If 0, then request is to all MPS units.
	if(*(messageP+3)=='V' && *(messageP+4)=='1' && *(messageP+5)=='?'){ 
		if(module_no==0){getSetVoltages();} // all set voltages requested
		else if(module_no>0 && module_no<6){getSetVoltage(module_no);} // set voltage requested
		else{Serial.println("OutOfRange");}
	}
	else if(*(messageP+3)=='M' && *(messageP+4)=='0' && *(messageP+5)=='?'){
		if(module_no==0){getActualVoltages();}//all actual voltages requested
		else if(module_no>0 && module_no<6){getActualVoltage(module_no);} // actual voltage requested
		else{Serial.println("OutOfRange");}
	}
	else if(*(messageP+3)=='M' && *(messageP+4)=='1' && *(messageP+5)=='?'){
		if(module_no==0){getCurrents();} //all currents requested
		else if(module_no>0 && module_no<6){getCurrent(module_no);} //current requested
		else{Serial.println("OutOfRange");}
	}
	else if(module_no>0 && module_no<6 && *(messageP+3)=='V' && *(messageP+4)=='1' && *(messageP+5)=='='){//update voltage requested
		// need to get the voltage char sequence into a double. First need to figure out how long message is.
		// the following few lines converts a char pointer to a \0 terminated char array.
		// Voltage starts at index 6.
		int i=0;
		char charVoltage[7]={};
		while(*(messageP+i+6)!=LF){
			charVoltage[i]=*(messageP+i+6);
			i++;
		}
		charVoltage[i-1]='\0';//replace the checksum with a char array terminator.	
		double voltage=atof(charVoltage);
		updateMPS(module_no, voltage);
		//Serial.println(voltage);
		*(voltages+module_no-1)=voltage;
	}
	/*else if(*(messageP+3)=='I' && *(messageP+4)=='D' && *(messageP+5)=='='){
		int new_address=*(messageP+6)-'0';
		int old_address=*(messageP+1)-'0';
		setAddress(old_address, new_address);
	}*/
	else if(*(messageP+1)=='R' && *(messageP+5)=='T'){ //<STX>RESET}<LF> will reset the arduino. Use if LCD stops responding.
		resetFunc();
	}
	else{
		Serial.println("InvalidRequest");
	}
}


