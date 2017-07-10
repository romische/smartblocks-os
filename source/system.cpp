#include "system.h"
#include "context.h"
#include "firmware.h"


#include <avr/interrupt.h>
#include <string.h>


/* initialisation of the static singleton */
System System::_system;

/* Constructor */
System::System(){}

int System::schedule_task(void* address, void* args){}



void System::run(){
	cli();    
    //set interrupt for timer0 at 100Hz (every 10 milliseconds)
    // see http://www.instructables.com/id/Arduino-Timer-Interrupts/ for calcul
    
    TCCR0A = _BV(COM0A1) | _BV(WGM01);
    TCCR0B = _BV(CS02) | _BV(CS00); //1024 prescaler
    OCR0A = 2*F_CPU / 1024 / (1000 / TICK_INTERVAL); // F_CPU = 8000000 but works at 16000000...??
    TIMSK0 = _BV(OCIE0A); // enable timer compare interrupt
    sei();
}


ISR(TIMER0_COMPA_vect) {
	Firmware::GetInstance().printTime();
}
