#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include "context.h"

#include <avr/interrupt.h>
#include <string.h>   //memset

/*
 *   A remark on MAX_TASKS
 * ATMega has 2048 bytes of RAM.
 * A part of it is used by the heap. You may want to adjust MAX_TASK depending on how great is 
 * your heap just before calling system.run(). 
 * If it is too big, the program will restart over and over.
 */


#define TASK_STACK_SIZE 256
#define MAX_TASKS 3
#define TICK_INTERVAL 10

#define stackPointer uint8_t*

struct task_definition {
    stackPointer sp;
    bool running;
    int sleep_time;
};
typedef struct task_definition task_definition;


class System {	
	
private:
	System();
	static System _system;
	
public:
	static System& instance() { return _system; }
	
	int run();
	int schedule_task(void* address, void* args);
	
	void exit_task();
	void yield();
	void sleep(int t);
	
};

#endif
