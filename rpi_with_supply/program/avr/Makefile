
all : test.bin burn
PART=attiny25
PROGPART=t25

CFLAGS=-g -Wall -Os -mmcu=$(PART) -DF_CPU=20000000UL
ASFLAGS:=$(CFLAGS)
CC=avr-gcc

test.bin : test.elf
	avr-objcopy -j .text -j .data -O binary test.elf test.bin 

test.elf : test.c
	avr-gcc -I  -g $(CFLAGS)   -mmcu=$(PART) -Wl,-Map,test.map -o $@ test.c -L/usr/lib/binutils/avr/2.18

test.lst : test.c
	avr-gcc -c -g -Wa,-a,-ad $(CFLAGS) test.c > $@

burn : test.bin
	../avrflasher 0x1e9108 16 w test.bin

#For PLL
burnfuses :
	avrdude -c usbtiny -p $(PROGPART) -U lfuse:w:0x41:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m 

burnfuses_legacy_timer_1 :
	avrdude -c usbtiny -p $(PROGPART) -U lfuse:w:0x53:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m 

burnfuses_default :
	avrdude -c usbtiny -p $(PROGPART) -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m 

readfuses :
	avrdude -c usbtiny -p $(PROGPART) -U hfuse:r:high.txt:b -U lfuse:r:low.txt:b

clean : 
	rm -f *~ high.txt low.txt test.hex test.map test.elf *.o sendpack.S makesendpack
