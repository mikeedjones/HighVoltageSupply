#ifndef SIMPLETIMERMAH_H
#define SIMPLETIMERMAH_H
#include <Arduino.h>
/*
 * This class sets up a tidy timer which allows the caller to check how much time has elapsed since it was last polled and report whether a minimum interval has been passed.
 * This allows events to be setup to occur at minimum set intervals within a block of code.
  */

class SimpleTimer { 
    volatile unsigned long start;
    volatile unsigned long current;
    volatile unsigned long timeOut;
    volatile bool autoReset;
  public:
    SimpleTimer();
    SimpleTimer(unsigned long t);
    void init(unsigned long t); //initialisation function to be used if the timer is created with the null constructor.
    long elapsed();// return the elapsed time.
    bool timedOut();// function to check whether the timer has timed out. If it has, reports true and resets the timer
    bool timedOut(bool RESET);
    void reset(); // reset the timer.      
};

SimpleTimer::SimpleTimer(){ //default null constructor that initialises to default 100ms interval
  timeOut=100;
  start=0;
  current=start;
  autoReset=true;
}

void SimpleTimer::init(unsigned long t){
  timeOut=t;
  start=millis();
  current=start;
}

SimpleTimer::SimpleTimer(unsigned long t){
  timeOut=t;
  start=millis();
  current=start;
}

long SimpleTimer::elapsed(){
  current=millis();
  return current-start;
}

bool SimpleTimer::timedOut(){
  if(elapsed() >= timeOut){
    return true;
  }
  else{
    return false;
  }
}

bool SimpleTimer::timedOut(bool RESET){
  if(elapsed() >= timeOut){
    if(RESET) reset();
    return true;
  }
	else{
    return false;
  }
}

void SimpleTimer::reset(){
  start=millis();
  current=start;
}

#endif SIMPLETIMERMAH_H
