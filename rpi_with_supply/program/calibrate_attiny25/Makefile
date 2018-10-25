
all : calibrate_attiny25.bin burn
PART=attiny25
PROGPART=t25

CFLAGS=-g -Wall -Os -mmcu=$(PART)
ASFLAGS:=$(CFLAGS)
CC=avr-gcc

calibrate_attiny25.bin : calibrate_attiny25.elf
	avr-objcopy -j .text -j .data -O binary calibrate_attiny25.elf calibrate_attiny25.bin 

calibrate_attiny25.elf : calibrate_attiny25.c
	avr-gcc -I  -g $(CFLAGS)   -mmcu=$(PART) -Wl,-Map,calibrate_attiny25.map -o $@ calibrate_attiny25.c -L/usr/lib/binutils/avr/2.18

calibrate_attiny25.lst : calibrate_attiny25.c
	avr-gcc -c -g -Wa,-a,-ad $(CFLAGS) calibrate_attiny25.c > $@

burn : calibrate_attiny25.bin
	#../avrflasher.alt 0x1e9007 16 c 0xffed32
	../avrflasher 0x1e9108 16 w calibrate_attiny25.bin

burnfuses_default :
	avrdude -c usbtiny -p $(PROGPART) -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m 

readfuses :
	avrdude -c usbtiny -p $(PROGPART) -U hfuse:r:high.txt:b -U lfuse:r:low.txt:b

clean : 
	rm -f *~ high.txt low.txt calibrate_attiny25.hex calibrate_attiny25.map calibrate_attiny25.elf *.o sendpack.S makesendpack
