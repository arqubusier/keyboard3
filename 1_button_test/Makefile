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

upload2:
	avrdude -p atmega32u4 -P /dev/ttyACM0 -c avr109 -U flash:w:build-$(BOARD_TAG)/$(TARGET).hex
