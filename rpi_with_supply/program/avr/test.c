#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

void delay_ms(uint32_t time) {
  uint32_t i;
  for (i = 0; i < time; i++) {
    _delay_ms(1);
  }
}

#define NOOP asm volatile("nop" ::)

void SendTick();
void SendAPacket();
void SendBPacket();
void SendCPacket();

static void setup_clock( void )
{
	/*Examine Page 33*/

	CLKPR = 0x80;	/*Setup CLKPCE to be receptive*/
	CLKPR = 0x00;	/*No scalar*/
	OSCCAL = 0xf0; //B8 is bottom E8 is top. 
}

#define WSPORT PORTB
#define WSPIN _BV(4)

#define SEND_WS( var ) \
				mask = 8; \
				v = var; \
				while( mask ) \
				{ \
					if( v & 0x80 )  \
					{ \
						WSPORT |= WSPIN;  mask--; v<<=1; NOOP; /** 6us (CHECK) */  \
						WSPORT &= ~WSPIN;  /* 6us */ \
					} \
					else \
					{ \
						WSPORT |= WSPIN;  /* .3 us */ \
						WSPORT &= ~WSPIN; NOOP; \
						mask--;  v<<=1;  /* loner */ \
					} \
					\
				}

#define MUX(x,y) ((x*y)>>8)

register uint8_t v asm( "r3" );
register uint8_t mask asm("r4" );

int main( )
{
	int i = 0, j;
	cli();
	setup_clock();

	DDRB = _BV(1) | _BV(3) | _BV(4);
	PORTB = 0;

	while(1)
	{
		PORTB &=~_BV(3);
		PORTB &=~_BV(4);
		_delay_ms(10);
		PORTB |= _BV(3);
		PORTB |= _BV(4);
		_delay_ms(10);
	}


	return 0;
} 
