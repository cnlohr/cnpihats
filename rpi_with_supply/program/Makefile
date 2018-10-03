all : avrflasher

burnslave :
	(cd avr && make && cd .. )

avrflasher : avrflasher.c gen_ios.c avr_softspi.c
	gcc -o $@ $^

clean :
	rm -rf avrflasher

