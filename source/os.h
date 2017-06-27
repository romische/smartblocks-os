#ifndef OS_H
#define OS_H

#include <stdint.h>
#include <stdio.h>


#define TASK_STACK_SIZE 256


class OS {	
	
public:
	static OS& GetInstance() { return _os;	}
	void switch_context(uint16_t* address);

private:
	OS();
	static OS _os;
};

#endif
