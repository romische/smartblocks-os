#include "os.h"
#include "context.h"
#include "firmware.h"


#include <avr/interrupt.h>
#include <string.h>



/* initialisation of the static singleton */
OS OS::_os;

/* Constructor */
OS::OS(){

}

uint8_t* tempSp;
uint8_t *stackTop, *tasksStackTop;
FILE* output_file;

void OS::switch_context(uint16_t* address){
	//cli();
	output_file = Firmware::GetInstance().m_psHUART;

    asm volatile("in r0, __SP_L__\nsts stackTop, r0\nin r0, __SP_H__\nsts stackTop+1, r0");
    //stacktop points to the main function 

    // reserve for the while loop
    stackTop -= 32;
    tasksStackTop = stackTop - 256;

    /* DO_SMTH_ELSE*/
    //save the sp of the currently running task...
   

     /*set the stack top for the ISR...
    asm volatile(
        "lds r16, stackTop\n"
        "lds r17, stackTop+1\n"
        "out __SP_L__, r16\n"
        "out __SP_H__, r17\n"
    );*/
    //this set the stacktop to be the one of main, so main is called again and again
    
 
    /* SWITCH TASK */
    uint8_t* top = tasksStackTop;
    tasksStackTop -= TASK_STACK_SIZE;
    memset(top - TASK_STACK_SIZE, 0, TASK_STACK_SIZE);

   //put in stack : address and arguments
   *(top) = ((uint16_t)address);
   *(top - 1) = (((uint16_t)address) >> 8);
   
    void* arg = nullptr;
	uint8_t *registerStartAddr = top - 3;
	*(registerStartAddr - 24) = ((uint16_t)arg);
	*(registerStartAddr - 25) = (((uint16_t)arg) >> 8);

   //set the value of tempSp and run.
   tempSp = top-35;
   //RESTORE_CONTEXT(tempSp)
   
   //fprintf(output_file, "-- tempSp : %p \r\n", tempSp);

   return;
}




