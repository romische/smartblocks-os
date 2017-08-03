#ifndef TIMER_H
#define TIMER_H
 
#include <stdint.h>
#include <avr/interrupt.h>

class CTimer {
   
public:
   static CTimer instance(){ return _timer;}
   uint32_t GetMilliseconds();

private:
   CTimer();
   static CTimer _timer;
};

#endif
