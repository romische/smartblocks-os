# smartblocks-os

Smartblocks-os is a master thesis project. It aims to add multitasking support to the smartblocks.
 
It consists of the basic smartblock firmware available here https://github.com/allsey87/smartblock-os-project, 
augmented with multitask support inspired by https://github.com/chrismoos/avr-os.


# hardware
Implemented first on a Redboard (Sparkfun) with microcontroller ATmega328 (the same as arduino uno), then on the smartblocks.
/!\ If you run on the Redboard, be sure to modify the ./run script as specified in it, and be aware that it will run 2 times faster (unless you change F_CPU to 2xF_CPU in the timer constructor and in system.run) 


# file organisation

 - *source* contains the source code
 - *toolchain* contains the programs necessary for compilation (avr-g++, etc), referred by the Makefile. 
 - *run* is a small bash file that compiles and run the software on the redboard. (needs avrdude and picocom installed)

# how to use it ?

See the [main](https://github.com/romische/smartblocks-os/blob/master/source/main.cpp) file for an example.


## --> create a task
For creating a task, you need to declare a function like
```
    void func();
    void func(void* address_referring_to_whatever_kind_of_arguments_i_want);  // with arguments
    static void func(void* args); //if the function has to be a member function of a class
```
Then you can add those functions to the system, then start them all with :
```
    System::instance().schedule_task((void*) func, (void*) my_arg );   //or Object.func if member function
    //then run                                                         //returns -1 if not enough room for the task
    System::instance().run(); 
```

Inside those functions you can use
```
    void exit_task();
    void yield();
    void sleep(int t);
```

If you need some initialization, I advise making one task "init" that will in turn add all the other tasks.

*Why?* Because too much stuffs before system.run() would override the memory reserved for the tasks leading to a stack overflow (probable sign : the system will execute repeatitively the beginning of your code).

## --> use lockable resources

For locking resources, we use a simple non-blocking spinlock.

```
    HardwareSerial::instance().lock();
    /* code */
    HardwareSerial::instance().unlock();
```
DO NOT use outside the multitask context.

## --> make an object lockable
Just make it inherit from Resource :
```
#include "resource.h"
class CHUARTController : public Resource {  ...
```

# further improvements

- In [system.cpp](https://github.com/romische/smartblocks-os/blob/master/source/system.cpp) : Make the reservation of stack space for the tasks be in the run() function instead of the constructor of System. The actual implementation may cause problem if too much stack space is used before calling run().

- Refactor the code in [system.cpp](https://github.com/romische/smartblocks-os/blob/master/source/system.cpp) to make it more C++ (it contains global variables, the ISR macro instead of a class representing interrupts, the void* pointers, etc)

- Add comments wherever there is assembly code

- In [system.cpp](https://github.com/romische/smartblocks-os/blob/master/source/system.cpp) : Make the sleep() function of System more exact. Now it can have an error of + or - 10 ms (for lightweight and easy design)

- Change all "cli() [...] sei()" into 
```
   uint8_t oldSREG = SREG;
   cli();
   /* code */
   SREG = oldSREG;
```
(because we don't know if interrupts are disabled or activated when calling cli)

- Globally, throw and catch error codes (for example the 3 functions in system who return void...)
