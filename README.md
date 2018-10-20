# Keyboard3

Keyboard firmware for a split keyboard using a Sparkfun pro micro and a mcp23017 i/o expander.

# Dependencies

* Arduino (avr-gcc and avrdude)
* Sparkfun arduino add-on
    https://github.com/sparkfun/SF32u4_boards/archive/master.zip
* The makefile includes files from:
    https://github.com/sudar/Arduino-Makefile

# Building

Set variables in the Makefile to point to the correct location of the arduino libraries and/or
arduino sketch-book

Build

    make

Upload

    make upload2

# Serial Monitoring
