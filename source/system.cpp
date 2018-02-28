#include "system.h"



stackPointer stackTop;
stackPointer tasksStackTop;
stackPointer tempSp;

task_definition tasks[MAX_TASKS];

int task_count;
int current_task;


void do_something_else();


/* initialisation of the static singleton */
System System::_system;

/* Constructor */
System::System(){
	asm volatile("in r0, __SP_L__\nsts stackTop, r0\nin r0, __SP_H__\nsts stackTop+1, r0");
    stackTop -= 32;
	tasksStackTop = stackTop - TASK_STACK_SIZE;
	
	task_count=0;
	current_task=-1;
}

/* 
 * Set interrupt for timer0 at 100Hz (every 10 milliseconds)
 *   (see http://www.instructables.com/id/Arduino-Timer-Interrupts/ for calcul)
 */
int System::run(){	
	cli();    
    TCCR0A = _BV(COM0A1) | _BV(WGM01);
    TCCR0B = _BV(CS02) | _BV(CS00); //1024 prescaler
    OCR0A = F_CPU / 1024 / (1000 / TICK_INTERVAL); 
    TIMSK0 = _BV(OCIE0A); // enable timer compare interrupt
    sei();
	
	while(true){}  //to wait when no task is running
	return 0;
}


/*
 * Adds a task in the array of tasks, and initiate its stack.
 * If the maximum number of tasks is not reached, it is straightforward
 * Otherwise we must erase a non running task if possible. If not possible, return -1.
 */
int System::schedule_task(void* address, void* args){
	int task_num;
	
	//if every slot is occupied (= we reached max tasks)
	if(task_count == MAX_TASKS){
		//look for a task that is finished and save its ID in finished_task
		int finished_task = -1;
		for(int i = 0; i<task_count; i++){
		   	if(!tasks[i].running){
		   		finished_task = i;
		   		break;
		   	}
		}
		// if no not running task has been found, return -1
		if (finished_task == -1)
			return -1;
		else
			task_num = finished_task;
	}
	// if there is still free slots then just take the next one
	else{
		task_num=task_count;

		task_count++;	
	}
		
	// Reserving space on the stack for the task (top and the 256 bytes below)  
	// & fill it with 0's
	stackPointer top = tasksStackTop - (task_num*TASK_STACK_SIZE);
	memset(top - TASK_STACK_SIZE, 0, TASK_STACK_SIZE); 

	//put in stack : address and arguments
	*(top) = ((uint16_t)address);
	*(top - 1) = (((uint16_t)address) >> 8);
	 
    stackPointer registerStartAddr = top - 3;
    *(registerStartAddr - 24) = ((uint16_t) args);
    *(registerStartAddr - 25) = (((uint16_t) args) >> 8);
	 
	//save the initial stackpointer for this task  
	cli();
	tasks[task_num].sp = top-35; // because RESTORE_CONTEXT will pop 33 values. 
								   // the 2 remaining values are the return address (aka PC)
	tasks[task_num].sleep_time=-1;
	tasks[task_num].running=true;
	
	sei();
	
	return 0;
}


/*
 * Set the variable "running" of the current task to be false, meaning it will not run anymore.
 * Then calls to do_something_else
 */
void System::exit_task(){
	tasks[current_task].running=false;
	do_something_else();
}

/*
 * "pause" the task by calling to do_something_else
 */
void System::yield(){
	do_something_else();
}


/*
 * Set the variable "sleep_time" of the current task to be t.
 * Then calls to do_something_else.
 *   remark: It's not something exact because round the sleep time by TICK_INTERVAL,
 *   plus the interrupt is not guaranteed to be triggered at the exact good time
 */
void System::sleep(int t){
	tasks[current_task].sleep_time=t;
	do_something_else();
}



/*
 * Decrement the variable "sleep_time" of each sleeping task.
 * Will be called by the ISR so every TICK_INTERVAL ms.
 */
void update_sleep_time(){
    for(int i = 0; i<task_count; i++){
	    if(tasks[i].running && tasks[i].sleep_time>0)
	    	tasks[i].sleep_time-= TICK_INTERVAL;
	}
}



/*
 * Returns the index in the task array of the next task to run, which must be running and not sleeping
 * If none is found, return -1
 */
int chooseNextTask(){
    int next = current_task;
    
    for(int i = 0; i<task_count; i++){
        next = (next+1)%task_count;
	    if(tasks[next].running && tasks[next].sleep_time<=0)
	    	return next;
	}

	// if no running task is found --> leading to undefined behavior
	return -1;
}


/*
 * Set the stack to run the next task
 */
void do_something_else() {
    cli();

    SAVE_CONTEXT(tempSp)
	
   
    if(current_task == -1)	stackTop = tempSp;
    else 					tasks[current_task].sp=tempSp;
		
	current_task = chooseNextTask();

    if(current_task == -1) 	tempSp = stackTop;
    else 				  	tempSp=tasks[current_task].sp;


    RESTORE_CONTEXT(tempSp)
    sei();
}


/*
 * Interrupt Service Routine of Timer0.
 */
ISR(TIMER0_COMPA_vect, ISR_NAKED) {
	cli();

    SAVE_CONTEXT(tempSp)
    
	update_sleep_time();
	
    if(current_task == -1)	stackTop = tempSp;
    else 					tasks[current_task].sp=tempSp;
		
	current_task = chooseNextTask();

    if(current_task == -1) 	tempSp = stackTop;
    else 				  	tempSp=tasks[current_task].sp;

    RESTORE_CONTEXT(tempSp)
    asm volatile("reti");
}
