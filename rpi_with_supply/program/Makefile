all : avrflasher avrflasher.alt


calandburn : cal burnslave

cal :
	./avrflasher 0x1e9108 16 c 0xffd7e2
	./avrflasher 0x1e9108 16 o 16000000

burnslave :
	(cd avr && make && cd .. )

avrflasher.alt : avrflasher.c gen_ios.c avr_softspi.c
	gcc -DALTERNATE_PINS_PGM -o $@ $^

avrflasher : avrflasher.c gen_ios.c avr_softspi.c
	gcc -o $@ $^

clean :
	rm -rf avrflasher avrflasher.alt


