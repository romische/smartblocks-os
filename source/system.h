#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include <stdio.h>


#define TASK_STACK_SIZE 256
#define MAX_TASKS 4
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
	void sleep(int t);
	
};

#endif
