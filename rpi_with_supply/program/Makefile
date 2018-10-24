all : avrflasher avrflasher.alt

burnslave :
	(cd avr && make && cd .. )

avrflasher.alt : avrflasher.c gen_ios.c avr_softspi.c
	gcc -DALTERNATE_PINS_PGM -o $@ $^

avrflasher : avrflasher.c gen_ios.c avr_softspi.c
	gcc -o $@ $^

clean :
	rm -rf avrflasher avrflasher.alt

