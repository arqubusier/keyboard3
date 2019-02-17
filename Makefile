# Modified from
#    https://stackoverflow.com/questions/33158180/how-to-use-arduino-makefile-with-sparkfun-pro-micro
BOARD_TAG         = promicro
ALTERNATE_CORE_PATH    = /home/herman/.arduino15/packages/SparkFun/hardware/avr/1.1.12
ALTERNATE_CORE    = sparkfun

ARDMK_DIR    	 = /home/herman/src/Arduino-Makefile
ARDMK_VENDOR	 = arduino
ARDUINO_DIR      = /home/herman/src/arduino-1.8.8
BOARD_SUB        = 16MHzatmega32U4

include $(ARDMK_DIR)/Arduino.mk

N_LAYERS=3
%.hpp: %.layout layoutgen.py
	./layoutgen.py ${N_LAYERS} $< $@
keyboard3.ino: left.hpp right.hpp

upload2: build-promicro-16MHzatmega32U4/keyboard3.hex
	avrdude -p atmega32u4 -P /dev/ttyACM0 -c avr109 -U flash:w:build-promicro-16MHzatmega32U4/keyboard3.hex
