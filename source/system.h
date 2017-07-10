#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include <stdio.h>


#define TASK_STACK_SIZE 256


class System {	
	
public:
	static System& GetInstance() { return _system; }

private:
	System();
	static System _system;
};

#endif
