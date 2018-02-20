#ifndef RESOURCE_H
#define RESOURCE_H

#include <avr/interrupt.h>
#include "system.h"

class Resource {	
	
public:

	//Resource() : locked(false) {}
	//seem to work without it...
	

	/* WARNING : this MUST be used in the context of multitask */
	void lock(){
		while (true){
			if(!locked){
				cli();
				if(!locked){
					locked=true;
					sei();
					return;
				}
				sei();
			}
			System::instance().yield();
		}
	}	
	
	void unlock(){
		cli();
		locked=false;
		sei();
		System::instance().yield();  //yield here to let other take the resource
	}

private:
	volatile bool locked = false;
};

#endif
