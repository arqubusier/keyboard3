# Keyboard3

Keyboard firmware for a (split) keyboard using a Sparkfun pro micro and a mcp23017 i/o expander.

# Dependencies

* Arduino (avr-gcc and avrdude)
* Sparkfun arduino add-on
    https://github.com/sparkfun/Arduino_Boards
* The makefile includes files from:
    https://github.com/sudar/Arduino-Makefile

# Building and Programming

Set variables in the Makefile to point to the correct location of the arduino libraries and/or
arduino sketch-book

## Build

    make

## Programming the arduino pro micro

To program the pro micro when a program is already running on it, the micro controller must be
reset so that the bootloader will run.
This is done by pulling the reset pin low twice in quick succession.
Once run, the bootloader will listen on the serial line for programming requests for eight seconds.
After this the program that is stored in flash, either an existing or a newly programmed program 
will be started by the bootloader.

Thus, after resetting the pro micro, issue (within eight seconds).

    make upload2

# Serial Monitoring (for debugging)

For example using picocom:

    picocom --baud 9600 <serial-device> 
