
/* AVR Headers */
#include <avr/io.h>
#include <avr/interrupt.h>


/* Firmware Headers */
#include <huart_controller.h>
#include <timer.h>
#include <system.h>


FILE* huart;

void dummy1(){
	for(int i=0; ;i++)
		printf(".%d.",i);
			//Firmware::GetInstance().GetHUARTController().Write('#');
}

void dummy2(void* arg){
	char c = *((char*) arg);
	while(true)
			HardwareSerial::instance().Write(c);
}

int main(void){
	
   stdout = HardwareSerial::instance().get_file();
   
   System::instance().schedule_task((void*) dummy1, nullptr);
   
   char arg[] = {'-', '~', '+', '.', 'o'};
   System::instance().schedule_task((void*) dummy2, (void*) arg ); //-
   System::instance().schedule_task((void*) dummy2, (void*) arg+1 ); //~
   System::instance().schedule_task((void*) dummy2, (void*) arg+2 ); //+
   //System::instance().schedule_task((void*) dummy2, (void*) arg+3 ); //.
   //System::instance().schedule_task((void*) dummy2, (void*) arg+4 ); //o
   
   
   
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

