#include "timer.h"



volatile uint32_t m_unTimerMilliseconds;

CTimer CTimer::_timer;


CTimer::CTimer(){
	m_unTimerMilliseconds=0;
	
	cli();    
    TCCR2A = _BV(COM2A1) | _BV(WGM21);
    TCCR2B = _BV(CS22); //64 prescaler
    OCR2A = 2*F_CPU / 64 / (1000 ); // F_CPU = 8000000 but works at 16000000...??
    TIMSK2 = _BV(OCIE2A); // enable timer compare interrupt
    sei();
}

   
uint32_t CTimer::GetMilliseconds(){

   uint32_t m;
   uint8_t oldSREG = SREG;

   cli();
   m = m_unTimerMilliseconds;
   SREG = oldSREG;

   return m;
}


ISR(TIMER2_COMPA_vect) {
	m_unTimerMilliseconds++;
}
