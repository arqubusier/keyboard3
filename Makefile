# Modified from
#    https://stackoverflow.com/questions/33158180/how-to-use-arduino-makefile-with-sparkfun-pro-micro
BOARD_TAG         = promicro16
ALTERNATE_CORE    = sparkfun

ARDMK_DIR    	 = /home/herman/src/Arduino-Makefile
ARDMK_VENDOR	 = arduino
ARDUINO_DIR      = /home/herman/arduino-1.8.7

include $(ARDMK_DIR)/Arduino.mk

N_LAYERS=3
%.hpp: %.layout layoutgen.py
	./layoutgen.py ${N_LAYERS} $< $@
keyboard3.ino: left.hpp right.hpp
upload2:
	avrdude -p atmega32u4 -P /dev/ttyACM0 -c avr109 -U flash:w:build-$(BOARD_TAG)/$(TARGET).hex
