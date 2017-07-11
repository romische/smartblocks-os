#include "system.h"
#include "context.h"
#include "firmware.h"


#include <avr/interrupt.h>
#include <string.h>



stackPointer stackTop;
stackPointer tasksStackTop;
stackPointer mainTask;

stackPointer tempSp;
stackPointer tasks_sp[MAX_TASKS];

int task_count;
int current_task;

/* initialisation of the static singleton */
System System::_system;

/* Constructor */
System::System(){
	asm volatile("in r0, __SP_L__\nsts stackTop, r0\nin r0, __SP_H__\nsts stackTop+1, r0");
    stackTop -= 32;
	tasksStackTop = stackTop - 256;
	
	task_count=0;
	current_task=-1;
	
}

int System::schedule_task(void* address, void* args){

	// Reserving space on the stack for the task
	stackPointer top = tasksStackTop;
	tasksStackTop -= TASK_STACK_SIZE;
	memset(top - TASK_STACK_SIZE, 0, TASK_STACK_SIZE);

	//put in stack : address and arguments
	*(top) = ((uint16_t)address);
	*(top - 1) = (((uint16_t)address) >> 8);
	   
	//save the initial stackpointer for this task
	tasks_sp[task_count] = top-35; // because SAVE_CONTEXT will pop 33 values. 
								   // the 2 remaining values are the return address (aka PC)
	task_count++;	
}



/* Set interrupt for timer0 at 100Hz (every 10 milliseconds)
 * see http://www.instructables.com/id/Arduino-Timer-Interrupts/ for calcul
 */
void startInterrupt(){
	cli();    
    TCCR0A = _BV(COM0A1) | _BV(WGM01);
    TCCR0B = _BV(CS02) | _BV(CS00); //1024 prescaler
    OCR0A = 2*F_CPU / 1024 / (1000 / TICK_INTERVAL); // F_CPU = 8000000 but works at 16000000...??
    TIMSK0 = _BV(OCIE0A); // enable timer compare interrupt
    sei();
}


int System::run(){	
	startInterrupt();

	while(true){}
	return 0;
}


int chooseNextTask(){
	return (current_task+1)%task_count;
}


ISR(TIMER0_COMPA_vect, ISR_NAKED) {
	SAVE_CONTEXT(tempSp)
	cli();

	if(current_task != -1)
		tasks_sp[current_task]=tempSp;
	
	
	current_task = chooseNextTask();
	tempSp=tasks_sp[current_task];
		
	
	
    RESTORE_CONTEXT(tempSp)
    asm volatile("reti");
}
