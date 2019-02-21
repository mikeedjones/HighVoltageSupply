#ifdef EI_SECTION_RISING
if (portNumber==PB) {
  risingPinsPORTB |= portMask;
}
if (portNumber==PJ) {
  risingPinsPORTJ |= portMask;
}
if (portNumber==PK) {
  risingPinsPORTK |= portMask;
}
#endif

#ifdef EI_SECTION_FALLING
if (portNumber==PB) {
  fallingPinsPORTB |= portMask;
}
if (portNumber==PJ) {
  fallingPinsPORTJ |= portMask;
}
if (portNumber==PK) {
  fallingPinsPORTK |= portMask;
}
#endif

#ifdef EI_SECTION_ASSIGNFUNCTIONSREGISTERS
if (portNumber==PB) {
#ifndef NEEDFORSPEED
  calculatedPointer=&portBFunctions.pinZero + portBitNumber;
  *calculatedPointer = userFunction;
#endif

  portSnapshotB=*portInputRegister(portNumber);
  pcmsk=&PCMSK0;
  PCICR |= _BV(0);
}
if (portNumber==PJ) {
#ifndef NEEDFORSPEED
  calculatedPointer=&portJFunctions.pinZero + portBitNumber;
  *calculatedPointer = userFunction;
#endif

  portSnapshotJ=*portInputRegister(portNumber);
  pcmsk=&PCMSK1;
  PCICR |= _BV(1);
  portJPCMSK|=portMask; // because PCMSK1 is shifted wrt. PortJ.
  portMask <<= 1; // Handle port J's oddness. PJ0 is actually 1 on PCMSK1.
}
if (portNumber==PK) {
#ifndef NEEDFORSPEED
  calculatedPointer=&portKFunctions.pinZero + portBitNumber;
  *calculatedPointer = userFunction;
#endif

  portSnapshotK=*portInputRegister(portNumber);
  pcmsk=&PCMSK2;
  PCICR |= _BV(2);
}
#endif

#ifdef EI_SECTION_DISABLEPINCHANGE
if (portNumber == PB) {
  PCMSK0 &= ~portMask;
  if (PCMSK0 == 0) { PCICR &= ~_BV(0); };
  risingPinsPORTB &= ~portMask;
  fallingPinsPORTB &= ~portMask;
}
if (portNumber == PJ) {
  // Handle port J's oddness. PJ0 is actually 1 on PCMSK1.
  PCMSK1 &= ((~portMask << 1) | 0x01); // or with 1 to not touch PE0.
  if (PCMSK1 == 0) { PCICR &= ~_BV(1); };
  risingPinsPORTJ &= ~portMask;
  fallingPinsPORTJ &= ~portMask;
}
if (portNumber == PK) {
  PCMSK2 &= ~portMask;
  if (PCMSK2 == 0) { PCICR &= ~_BV(2); };
  risingPinsPORTK &= ~portMask;
  fallingPinsPORTK &= ~portMask;
}
#endif

