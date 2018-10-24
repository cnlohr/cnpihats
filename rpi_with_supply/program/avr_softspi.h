#ifndef _AVR_SOFTSPI_H
#define _AVR_SOFTSPI_H

#include <stdint.h>
#include "gen_ios.h"

#ifdef ALTERNATE_PINS_PGM
#define GP_AVR_RST  2
#define GP_AVR_MISO 17
#define GP_AVR_MOSI 3
#define GP_AVR_SCK  4
#else
#define GP_AVR_RST  8
#define GP_AVR_MISO 9
#define GP_AVR_MOSI 10
#define GP_AVR_SCK  11
#endif

#define AVRSCKON  GPIOSet( GP_AVR_SCK, 1 );
#define AVRSCKOFF GPIOSet( GP_AVR_SCK, 0 );
#define AVRSETRST( x )  GPIOSet( GP_AVR_RST, x );
#define AVRSETMOSI( x ) GPIOSet( GP_AVR_MOSI, x );
#define AVRGETMISO      (GPIOGet( GP_AVR_MISO ))
#define INIT_GPIOS() { InitGenGPIO(); GPIOSet( GP_AVR_RST, 1 ); GPIODirection( GP_AVR_RST, 1 ); GPIODirection( GP_AVR_MISO, 0 ); GPIODirection( GP_AVR_MOSI, 1 ); GPIODirection( GP_AVR_SCK, 1 ); }
#define UNINIT_GPIOS() { GPIODirection( GP_AVR_RST, 0 ); GPIODirection( GP_AVR_MISO, 0 ); GPIODirection( GP_AVR_MOSI, 0 ); GPIODirection( GP_AVR_SCK, 0 ); }

#define AVR_DEBUG_VERIFY

static void avrdelay() { uint8_t i;	for( i = 0; i < 100; i++ ) asm("nop"); }


void InitAVRSoftSPI();
//void TickAVRSoftSPI( int slow );	//Don't call this??
int  ProgramAVRFlash( uint8_t * source, uint16_t bytes, int signature, int pagesize ); //Signature is sig2 MSB and sig0 LSB
void ResetAVR();
int  EraseAVR();
int DumpAVRMemories( int procid );
int  Burnfuses( uint32_t procid, uint32_t hfuselfuse );
int  ReadFlashWord( int procid, int address );
int  WriteEEProm( uint32_t procid, int address, uint8_t value );
void UnconfigureAVRPins();


#endif

