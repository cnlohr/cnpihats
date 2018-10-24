//AVR Programming and serial communications interface, loosely based off of AVR910.

#include "avr_softspi.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>


//NOTES: When reading, data is clocked on the falling edge of SCK
//ESP8266 has a LSB first endian.  The bytes are MSB first.  We're just going to
//ignore that fact.

#define ERASE_TIMEOUT 25 //Roughly in MS.
#define FLASH_PAGE_SIZE_WORDS 8

static uint8_t AVRSR( uint8_t data )
{
	uint8_t ret = 0;
	uint8_t i = 0x80;

	usleep(1);  //Is 8 cycles enough to service the interrupt on the AVR?
	for( ; i != 0; i>>=1 )
	{
		AVRSETMOSI( ((i&data)!=0) )
		avrdelay();
		AVRSCKON
		avrdelay();
		ret |= AVRGETMISO?i:0;
		AVRSCKOFF
	}

	return ret;
}

static uint32_t  AVRSR4( uint32_t data )
{
	uint32_t ret = 0;
	uint32_t i = 0x80000000;
	for( ; i != 0; i>>=1 )
	{
		AVRSETMOSI( ((i&data)!=0) )
		usleep(2);
		AVRSCKON
		usleep(2);
		ret |= AVRGETMISO?i:0;
		AVRSCKOFF
	}

	usleep(8); //Too short?

	return ret;
}

static int  EnterAVRProgramMode( int midid )//0x92 = ATTiny441 0x93 = ATTiny841 0x91 = ATTiny85
{
	uint32_t rr;

	AVRSCKOFF;
	usleep(5);
	AVRSETRST(1);
	usleep(20);
	AVRSETRST(0);
	usleep(20*1000); //XXX TODO try to reduce this?

	rr = AVRSR4( 0xAC530000 );
	if( ( rr & 0x0000ff00 ) != 0x00005300 )
	{
		printf( "RR: %08x\n", rr );
		return 0xffff0000;
	}

	if( ( rr = AVRSR4( 0x30000000 ) & 0xff ) != (midid>>16&0xff) )
	{
		return 0xffff1000 | ( rr & 0xff );
	}

	if( ( rr = AVRSR4( 0x30000100 ) & 0xff ) != ((midid>>8)&0xff) ) 
	{
		return 0xffff2000 | ( rr & 0xff );
	}

	if( ( rr = AVRSR4( 0x30000200 ) & 0xff ) != ((midid)&0xff) )
	{
		return 0xffff3000 | ( rr & 0xff );
	}

	return 0;
}

static int  WaitForAVR()
{
	int i;
	for( i = 0; i < ERASE_TIMEOUT*10; i++ )
	{
		if( ( AVRSR4(0xF0000000) & 1 )  == 0 ) return 0;
		usleep(100);
	}
	return 0xFFFFBEEF;
}


int  EraseAVR()
{
	int i;
	int r = AVRSR4( 0xAC800000 );

	if( WaitForAVR() ) return 0xFFFF5000;

	return 0;
}


void  ResetAVR()
{
	AVRSETRST(0);
	usleep(10);
	AVRSETRST(1);
	usleep(20*1000);
}



int  ProgramAVRFlash( uint8_t * source, uint16_t bytes, int procid, int pagesize )
{
	int i;
	int r;

	r = EnterAVRProgramMode( procid ); 	if( r ) { printf( "Enter Program Mode Fail.\n" ); goto fail; }
	r = EraseAVR(); 			if( r ) { printf( "Erase fail.\n" ); goto fail; }
	usleep(2000);
	printf( "Programming AVR %p:%d\n", source, bytes );
	int bytesin = 0;
	int blockno = 0;

	while( bytes - bytesin > 0 )
	{
		uint8_t buffer[pagesize*2];
		int wordsthis = (bytes - bytesin + 1) / 2;
		if( wordsthis > pagesize ) wordsthis = pagesize;

		memcpy( buffer, &source[bytesin], pagesize*2 );
		printf( "%02x %02x\n", buffer[0], buffer[1] );
		for( i = 0 ; i < wordsthis; i++ )
		{
			AVRSR4( 0x40000000 | (i << 8) | buffer[i*2+0] );
			AVRSR4( 0x48000000 | (i << 8) | buffer[i*2+1] );
			//printf( "%02x%02x ",buffer[i*2+0],buffer[i*2+1] );
		}

		r = AVRSR4( 0x4C000000 | ((blockno*pagesize)<<(8)) );
		if( WaitForAVR() ) return -9;

		blockno++;
		bytesin+=pagesize*2;
	}

	usleep(2000);

#ifdef AVR_DEBUG_VERIFY
	printf("\nReading\n" );
	for( i = 0; i < (bytes+1)/2; i++ )
	{
		uint8_t ir = AVRSR4( 0x20000000 | ((i)<<8 ) )&0xff;
		printf( "%02x", ir );
		ir = AVRSR4( 0x28000000 | ((i)<<8) )&0xff;
		printf( "%02x ", ir );
		if( ( i & 0x07 ) == 0x07 ) printf( "\n" ); 
	}
#endif

	printf( "Reset AVR.\n" );
	AVRSETRST(1);
	usleep(20*1000);

	return 0;
fail:
	//ets_wdt_enable();
	printf( "Program fail: %08x\n", r );
	return -1;
}

int  Burnfuses( uint32_t procid, uint32_t hfuselfuse )
{
	int r = EnterAVRProgramMode( procid ); 	if( r ) { printf( "Enter Program Mode Fail.\n" ); goto fail; }
	usleep(2000);
	uint8_t hfuse = (hfuselfuse & 0xff00) >> 8;
	uint8_t lfuse = hfuselfuse & 0xff;
	printf( "Programming AVR %p:%02x:%02x\n", procid, hfuse, lfuse );
	AVRSR4( 0x58080000 );
	AVRSR4( 0x50000000 );	//Prep chip.

	int ret = 0;
	int tries = 0;

	for( tries; tries < 10; tries++ )
	{
		AVRSR4( 0xaca80000 | hfuse); 
		usleep( 100 );
		ret = AVRSR4( 0x58080000 ) & 0xff;
		ret = AVRSR4( 0x58080000 ) & 0xff;
		ret = AVRSR4( 0x58080000 ) & 0xff;
		if( ret == hfuse ) break;
	}
	if( ret != hfuse ) { fprintf( stderr, "Error: Couldn't write HFUSE [%02x != %02x]\n", hfuse, ret ); goto fail; }

	for( tries; tries < 10; tries++ )
	{
		AVRSR4( 0xaca00000 | lfuse);
		usleep( 100 );
		ret = AVRSR4( 0x50000000 ) & 0xff;
		ret = AVRSR4( 0x50000000 ) & 0xff;
		ret = AVRSR4( 0x50000000 ) & 0xff;
		if( ret == lfuse ) break;
	}
	if( ret != lfuse ) { fprintf( stderr, "Error: Couldn't write LFUSE [%02x != %02x]\n", lfuse, ret ); goto fail; }

	hfuse = AVRSR4( 0x58080000 );
	lfuse = AVRSR4( 0x50000000 );
	printf( "Read back: %02x:%02x\n",  hfuse&0xff, lfuse&0xff );

	printf( "Reset AVR.\n" );
	AVRSETRST(1);
	usleep(20*1000);

	return 0;
fail:
	return -1;
}


int  WriteEEProm( uint32_t procid, int address, uint8_t value )
{
	int r = EnterAVRProgramMode( procid ); 	if( r ) { printf( "Enter Program Mode Fail.\n" ); goto fail; }
	usleep(2000);
	AVRSR4( 0xc0000000 | (address<<8) | value );
	uint8_t rvalue = AVRSR4( 0xa0000000 | (address<<8) );
	printf( "Read back: %02x:%02x\n",  address, rvalue );

	printf( "Reset AVR.\n" );
	AVRSETRST(1);
	usleep(20*1000);
	return 0;
fail:
	return -1;
}

int DumpAVRMemories( int procid )
{
	int r = EnterAVRProgramMode( procid ); 	if( r ) { printf( "Enter Program Mode Fail.\n" ); goto fail; }
	int i;
	uint32_t hfuse = AVRSR4( 0x58080000 );
	uint32_t lfuse = AVRSR4( 0x50000000 );
	printf( "Read back: %02x:%02x\n",  hfuse&0xff, lfuse&0xff );

	printf("\nReading\n" );
	for( i = 0; i < 512; i++ ) //XXX WARNING: Fix size?
	{
		uint8_t ir = AVRSR4( 0x20000000 | ((i)<<8 ) )&0xff;
		printf( "%02x", ir );
		ir = AVRSR4( 0x28000000 | ((i)<<8) )&0xff;
		printf( "%02x ", ir );
		if( ( i & 0x07 ) == 0x07 ) printf( "\n" ); 
	}

	printf( "Reset AVR.\n" );
	AVRSETRST(1);
	usleep(20*1000);

	return 0;
fail:
	return -1;
}

int  ReadFlashWord( int procid, int address )
{
	int r = EnterAVRProgramMode( procid ); 	if( r ) { printf( "Enter Program Mode Fail.\n" ); goto fail; }

	int i = address >> 1;
	uint8_t irh = AVRSR4( 0x20000000 | ((i)<<8 ) )&0xff;
	uint8_t irl = AVRSR4( 0x28000000 | ((i)<<8) )&0xff;
	AVRSETRST(1);
	usleep(20*1000);
	return (address &1)?irl:irh;
fail:
	return -1;
}

void InitAVRSoftSPI()
{
	INIT_GPIOS();
	AVRSETRST(0);
	usleep(30);
	AVRSETRST(1);
	usleep(20*1000);
}

void UnconfigureAVRPins()
{
	UNINIT_GPIOS();
}

#if 0

int wason = 0;

void  TickAVRSoftSPI( int slow )
{
//	if( !slow ) return;

//	int ir = AVRSR( 'T' );

	int t;

	for( t = 0; t < 10; t++ )
	{
		int ir = AVRSR( 0 );
		if( ir )
		{
			int j = 0;
			printf( "%02x: ", ir );
			ir &= 0x0f;
			for( j = 0; j < ir; j++ )
			{
				printf( "%c", AVRSR( 0 ) );
			}
			printf( "\n" );
		}
		else
		{
			break;
		}
	}

//	printf( "\n" );
/*	static int frame;
	if( !slow ) return;

	frame++;

	if( wason )
	{
		AVRSETMOSI(1)
	}
	else
	{
		AVRSETMOSI(0)
	}
	
	printf( "%d %d\n", wason, AVRGETMISO );
	if( ( frame & 0xf ) == 0 )
	{
		wason = !wason;
		ProgramFlash();
	}*/
}

#endif



