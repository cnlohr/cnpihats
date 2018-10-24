
all : calibrate_attiny13.bin burn
PART=attiny13
PROGPART=t13

CFLAGS=-g -Wall -Os -mmcu=$(PART)
ASFLAGS:=$(CFLAGS)
CC=avr-gcc

calibrate_attiny13.bin : calibrate_attiny13.elf
	avr-objcopy -j .text -j .data -O binary calibrate_attiny13.elf calibrate_attiny13.bin 

calibrate_attiny13.elf : calibrate_attiny13.c
	avr-gcc -I  -g $(CFLAGS)   -mmcu=$(PART) -Wl,-Map,calibrate_attiny13.map -o $@ calibrate_attiny13.c -L/usr/lib/binutils/avr/2.18

calibrate_attiny13.lst : calibrate_attiny13.c
	avr-gcc -c -g -Wa,-a,-ad $(CFLAGS) calibrate_attiny13.c > $@

burn : calibrate_attiny13.bin
	../avrflasher.alt 0x1e9007 16 c 0xffed32
	../avrflasher.alt 0x1e9007 16 w calibrate_attiny13.bin

burnfuses_default :
	avrdude -c usbtiny -p $(PROGPART) -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m 

readfuses :
	avrdude -c usbtiny -p $(PROGPART) -U hfuse:r:high.txt:b -U lfuse:r:low.txt:b

clean : 
	rm -f *~ high.txt low.txt calibrate_attiny13.hex calibrate_attiny13.map calibrate_attiny13.elf *.o sendpack.S makesendpack
