#include <stdio.h>
#include "avr_softspi.h"


int main( int argc, char ** argv )
{
	if( argc !=2 )
	{
		fprintf( stderr, "Error: Usage: %s [.bin file]\n", argv[0] );
		return -1;
	}

	FILE * f = fopen( argv[1], "rb" );
	fseek( f, 0, SEEK_END );
	int len = ftell( f );
	fseek( f, 0, SEEK_SET );

	uint8_t buffer[len];
	fread( buffer, len, 1, f );
	fclose( f );
	
	InitAVRSoftSPI();
	int r = ProgramAVRFlash( buffer, len, 0x1e9108, 16 ); //attiny25
	printf( "Got code %d\n", r );
	return r;
}

