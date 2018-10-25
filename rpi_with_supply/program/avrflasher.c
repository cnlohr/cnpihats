#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>

#include "avr_softspi.h"

//Returns zero of no error, otherwise error code.
int cnreadint( const char * pl, int * number, const char ** ptpl )
{
	int retcode = 0;
	if( !pl ) return -2;

	int negative = 0;
	int didhit = 0;
	int retval = 0;
	int base = 10;
	int first = 1;
	char c;

	while( (c = *(pl++)) )
	{
		if( c == ' ' || c == '\t' || c == '+' ) continue;
		else if( c == '-' ) { negative = 1; continue; }
		else if( c >= '0' && c <= '9' ) { break; }
		else goto endmark;
	}

	if( c == '0' )
	{
		//Octal, Hex or Binary
		c = *(pl++);
		if( c == 'x' ) { base = 16; c = *(pl++); }
		else if( c == 'b' ) { base = 2; c = *(pl++); }
		else if( c >= '0' && c <= '7' ) base = 8;
		else if( c == 0 ) { retcode = 0; goto endmark; }
		else { retcode = -3; goto endmark; }
	}

	if( c == 0 )
	{
		retcode = -5;
		goto endmark;
	}

	do
	{
		int dec = 0;
		if( c >= '0' && c <= '9' ) dec = c - '0';
		else if( c >= 'a' && c <= 'f' ) dec = c - 'a' + 10;
		else if( c >= 'A' && c <= 'F' ) dec = c - 'A' + 10;
 		else goto endmark;
		if( dec >= base ) goto endmark;

		retval = retval * base + dec;
	
		c = *(pl++);
	} while( c );

	pl--;

endmark:
	if( retcode == 0 )
	{
		*number = retval;
	}
	if( ptpl )
		*ptpl = pl;

	return retcode;
}

double GTime()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec + tv.tv_usec / 1000000.0;
}



//This assumes you are NOT in programming mode.
double GetMHzOfOSCCAL( int devcode, int pagesizewords, uint8_t osccal )
{
	int i;

	InitAVRSoftSPI();
	WriteEEProm( devcode, 0, osccal );
	UnconfigureAVRPins();


	//wait for line to go low.
	double Last = GTime();
	double Now = GTime();
	int tries = 5;
	double Times[tries];
	double TotalTimes = 0;

	for( i = 0; i < tries; i++ )
	{
		Last = GTime();
		while( GPIOGet( GP_AVR_MOSI ) && ( (Now = GTime()) - Last ) < 1.0 );
		if( Now-Last >= 1.0 ) goto timeout;
		Last = GTime();
		while( !GPIOGet( GP_AVR_MOSI ) && ( (Now = GTime()) - Last ) < 1.0 );
		if( Now-Last >= 1.0 ) goto timeout;
		Times[i] = Now-Last;
		TotalTimes += Now - Last;
	}

	TotalTimes /= tries;

	double mostaccuratediff = 1e20;
	double mostaccurate = 0;
	//Find time closest to average.
	for( i = 0; i < tries; i++ )
	{
		double diff = Times[i] - TotalTimes;
		if( diff < 0 ) diff = -diff;
		if( diff < mostaccuratediff )
		{
			mostaccuratediff = diff;
			mostaccurate = Times[i];
		}
		printf( "%f ", 1.0/(Times[i]/50000.0) );
	}
	printf( "\n" );

	double MHz = 1.0/(mostaccurate/50000.0);
	return MHz;

timeout:
	fprintf( stderr, "Error: Timeout on timer\n" );
	return -5;
}


int main( int argc, char ** argv )
{
	if( argc < 4 )
	{
		fprintf( stderr, "Error: Usage: %s [devcode] [pagesizewords] [wecrbo] [Parameters]\n", argv[0] );
		fprintf( stderr, " 	  devcode = 0x1e9108 16 = ATTiny25\n\
	   devcode = 0x1e9007 16 = ATTiny13\n\
	w: write flash.  Extra parameter  [binary file to flash]\n\
	e: erase chip.\n\
	c: config chip fuses [0xff_hfuse_lfuse] (must indicate 0xff first)\n\
	r: dump chip memories\n\
	b: read byte [address]\n\
	o: oscillator calibration. Extra par [target Hz, optional] (ATTINY13/ATTINY25 ONLY)\n");
		return -1;
	}

	int devcode = 0;
	int pagesizewords = 0;

	if( cnreadint( argv[1], &devcode, 0 ) )
	{
		fprintf( stderr, "Error: Can't decode dev id %s\n", argv[1] );
		return -2;
	}

	if( cnreadint( argv[2], &pagesizewords, 0 ) )
	{
		fprintf( stderr, "Error: Can't decode pagesize %s\n", argv[1] );
		return -2;
	}

	switch( argv[3][0] )
	{
	case 'w': case 'W':
	{
		if( argc != 5 )
		{
			fprintf( stderr, "Write operation need binary file\n" );
			return -9;
		}
		FILE * f = fopen( argv[4], "rb" );
		if( !f )
		{
			fprintf( stderr, "Error can't open %s for flashing\n", argv[4] );
			return -3;
		}
		fseek( f, 0, SEEK_END );
		int len = ftell( f );
		fseek( f, 0, SEEK_SET );

		uint8_t buffer[len];
		fread( buffer, len, 1, f );
		fclose( f );

		InitAVRSoftSPI();
		int r = ProgramAVRFlash( buffer, len, devcode, pagesizewords ); //attiny25 = 0x1e9108
		printf( "Got code %d\n", r );
		UnconfigureAVRPins();
		return r;
	}
	case 'e': case 'E':
	{
		InitAVRSoftSPI();
		int r = EraseAVR();
		fprintf( stderr, "EraseAVR = %d\n", r );
		UnconfigureAVRPins();
		return r;
	}
	case 'c': case 'C':
	{
		int fusecode = 0;
		
		if( argc == 5 )
		{
			int r = cnreadint( argv[4], &fusecode, 0 );
			if( r ) fusecode = 0;
		}
		if( argc != 5 || (fusecode & 0xff0000) != 0xff0000 )
		{
			fprintf( stderr, "Need [0xff_hfuse_lfuse] to write.\n" );
			return -9;
		}
		InitAVRSoftSPI();
		int r = Burnfuses( devcode, fusecode & 0xffff );
		UnconfigureAVRPins();
		return r;		
	}
	case 'r': case 'R':
	{
		InitAVRSoftSPI();
		DumpAVRMemories(devcode);
		UnconfigureAVRPins();
		return 0;
	}
	case 'b': case 'B':
	{
		int raddr = -1; 
		if( argc == 5 )
		{
			int r = cnreadint( argv[4], &raddr, 0 );
			if( r ) raddr = -1;
		}
		if( argc != 5 || raddr < 0 )
		{
			fprintf( stderr, "Need [address] to write.\n" );
			return -9;
		}
		InitAVRSoftSPI();
		int r = ReadFlashWord( devcode, raddr );
		printf( "%d\n", r );
		UnconfigureAVRPins();
		return (r<0)?r:0;
	}
	case 'o': case 'O':
	{
		double targmhz = 8000000;
		if( argc == 5 )
		{
			targmhz = atof( argv[4] );
		}
		if( targmhz < .1 ) 
		{
			fprintf( stderr, "Error: need valid MHz parameter if specified\n" );
			return -5;
		}

		int osccal = 0x40;
		int jump = 0x40;
		int tries = 0;

		int best_osc = 0x80;
		double best_cal = 1e20;
		double freqat = 1e20;

		int calmax = 127;
		const char * flashdev = "DEVICE NOT SUPPORTED";
		if( devcode == 0x1e9108 )
		{
			flashdev = "calibrate_attiny25/calibrate_attiny25.bin";
			calmax = 255;
		}
		else if( devcode == 0x1e9007 )
		{
			flashdev = "calibrate_attiny13/calibrate_attiny13.bin";
		}

		FILE * f = fopen( flashdev, "rb" );
		if( !f )
		{
			fprintf( stderr, "Error can't open %s for flashing\n", flashdev );
			return -3;
		}

		fseek( f, 0, SEEK_END );
		int len = ftell( f );
		fseek( f, 0, SEEK_SET );
		uint8_t buffer[len];	
		fread( buffer, len, 1, f );
		fclose( f );
		InitAVRSoftSPI();
		int r = ProgramAVRFlash( buffer, len, devcode, pagesizewords ); //attiny25 = 0x1e9108
		printf( "Got code %d\n", r );
		UnconfigureAVRPins();


		for( tries = 0; tries < 16; tries++ )
		{
			double MHz = GetMHzOfOSCCAL( devcode, pagesizewords, osccal );
			if( MHz < 0 ) return -98;
			jump = jump * .8;
			printf( "CONFIG: %d, %f\n", osccal, MHz );
			double diff = MHz - targmhz;
			if( diff < 0 ) diff = -diff;
			if( diff < best_cal ) {
				best_osc = osccal;
				best_cal = diff;
				freqat = MHz;
			} 


			if( MHz < targmhz ) osccal += jump;
			else osccal -= jump;
			if( osccal < 0 ) osccal = 0;
			if( osccal > calmax ) osccal = calmax;
		}
		printf( "%d, %f, %f, %3.4f%%\n", best_osc, freqat, best_cal, best_cal/freqat*100. );
		return 0;
	}
	}
	return 0;
}

