# smartblocks-os

smartblocks-os is a master thesis project. It aims to add multitasking support to the smartblocks.
 
It consists of the basic smartblock firmware available here https://github.com/allsey87/smartblock-os-project, 
augmented with multitask support inspired https://github.com/chrismoos/avr-os.

It doesn't work yet.


# hardware

Implemented on a Redboard (Sparkfun) with microcontroller ATmega328 (the same as arduino uno).


# file organisation

 - *source* contains the source code
 - *toolchain* contains the programs necessary for compilation (avr-g++, etc), referred by the Makefile. 
 - *run* is a small bash file that compiles and run the software on the redboard. (needs avrdude and picocom)
 
