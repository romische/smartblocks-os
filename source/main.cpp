
/* AVR Headers */
#include <avr/io.h>
#include <avr/interrupt.h>


/* Firmware Headers */
#include "huart_controller.h"
#include "timer.h"
#include "system.h"


void dummy2(void* arg){
	char c = *((char*) arg);
	while(true){
		HardwareSerial::instance().lock();
		HardwareSerial::instance().Write(c);
		HardwareSerial::instance().unlock();
		System::instance().yield();
	}
}

void dummy1(){
	for(int i=0; ;i++){
		HardwareSerial::instance().lock();
		printf("%d_",i);
		HardwareSerial::instance().unlock();
		System::instance().sleep(100);
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


void dummy5(){
	CTimer timer = CTimer::instance();
	int millis;
	while(true){
		HardwareSerial::instance().lock();
		printf("Uptime = %d\r\n", timer.GetMilliseconds());
		HardwareSerial::instance().unlock();
		System::instance().sleep(1000);
	}
}

int main(void){
	
   stdout = HardwareSerial::instance().get_file();
   
   
   System::instance().schedule_task((void*) dummy5, nullptr);
   
   char arg[] = {'#', 'o', '-', '>', '~'};
   System::instance().schedule_task((void*) dummy2, (void*) arg ); //#
   System::instance().schedule_task((void*) dummy2, (void*) arg+1 ); //o
   System::instance().schedule_task((void*) dummy2, (void*) arg+2 ); //-
   System::instance().schedule_task((void*) dummy2, (void*) arg+3 ); //>
   System::instance().schedule_task((void*) dummy2, (void*) arg+4 ); //~
   
   
   //System::instance().schedule_task((void*) dummy3, nullptr);
   
   printf("\r\n\r\nStarting the program...\r\n");
   return System::instance().run();  
   
}


