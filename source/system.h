#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include <stdio.h>


#define TASK_STACK_SIZE 256
#define MAX_TASKS 5
#define TICK_INTERVAL 10

#define stackPointer uint8_t*

class System {	
	
public:
	static System& instance() { return _system; }
	
	int schedule_task(void* address, void* args);
	//void exit_task();
	//void sleep();
	
	void run();

private:
	System();
	static System _system;
};

#endif
