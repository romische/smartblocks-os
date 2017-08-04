
/* AVR Headers */
#include <avr/io.h>
#include <avr/interrupt.h>


/* Firmware Headers */
#include "huart_controller.h"
#include "timer.h"
#include "system.h"


void wait(){
	for(int j=0; j<1000; j++){
		asm volatile("nop");
	}

}



void dummy2(void* arg){
	char c = *((char*) arg);
	while(true){
		HardwareSerial::instance().lock();
		HardwareSerial::instance().Write(c);
		HardwareSerial::instance().unlock();
		wait();
	}
}

void dummy1(){
	for(int i=0; ;i++){
		HardwareSerial::instance().lock();
		printf("%d_",i);
		HardwareSerial::instance().unlock();
		wait();
		if(i==100){
			char arg = '6';
			System::instance().schedule_task((void*) dummy2, (void*) &arg ); 
		}
	}
}

void dummy3(){
	HardwareSerial::instance().lock();
	printf("dummy3 ran and ended\r\n");
	HardwareSerial::instance().unlock();
	System::instance().exit_task();
}

void dummy4(){
	CTimer timer = CTimer::instance();
	int millis;
	int last = (int) timer.GetMilliseconds();
	while(true){
		millis = (int) timer.GetMilliseconds();
		if(millis-last >1000){
			last = millis;
			HardwareSerial::instance().lock();
			printf("Uptime = %d\r\n", millis);
			HardwareSerial::instance().unlock();
		}
	}
}

int main(void){
	
   stdout = HardwareSerial::instance().get_file();
   
   //
   
   System::instance().schedule_task((void*) dummy4, nullptr);
   
   char arg[] = {'-', 'o', '+', '.', '~'};
   System::instance().schedule_task((void*) dummy2, (void*) arg ); //-
   System::instance().schedule_task((void*) dummy2, (void*) arg+1 ); //o
   System::instance().schedule_task((void*) dummy2, (void*) arg+2 ); //+
   System::instance().schedule_task((void*) dummy2, (void*) arg+3 ); //.
   System::instance().schedule_task((void*) dummy2, (void*) arg+4 ); //~
   
   
   //System::instance().schedule_task((void*) dummy3, nullptr);
   
   printf("\r\n\r\nStarting the program...\r\n");
   return System::instance().run();  
   
}




/*
to initialize the timer : 

m_cTimer(TCCR2A,
               TCCR2A | (1 << WGM21) | (1 << WGM20),
               TCCR2B,
               TCCR2B | (1 << CS22),
               TIMSK2,
               TIMSK2 | (1 << TOIE2),
               TIFR2,
               TCNT2,
               TIMER2_OVF_vect_num)


*/

