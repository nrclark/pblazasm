
/*
 *  Copyright © 2003..2012 : Henk van Kampen <henk@mediatronix.com>
 *
 *  This file is part of pBlazBIT.
 *
 *  pBlazBIT is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pBlazBIT is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pBlazBIT.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>


#include "pbTypes.h"
#include "pbCRC32tab.h"
#include "bsParse.h"
#include "pbS3E.h"

const char * sRegisterNames_S3[] = {
	"CRC",   // R/W 00000
	"FAR",   // R/W 00001
	"FDRI",  //   W 00010
	"FDRO",  // R   00011
	"CMD",   // R/W 00100
	"CTL",   // R/W 00101
	"MASK",  // R/W 00110
	"STAT",  // R   00111
	"LOUT",  //   W 01000
	"COR",   // R/W 01001
	"MFWR",  //   W 01010
	"FLR",   // R/W 01011
	"–",     // –   01100
	"–",     // –   01101
	"IDCODE" // R/W 01110
} ;

const char * sRegisterNames_S6[] = {
	"CRC = 0",          //00000
	"FARMAJ",           //00001
	"FARMIN",           //00010
	"FDRI",             //00011
	"FDRO",             //00100
	"CMD = 5",          //00101
	"CTL",              //00110
	"MASK",             //00111
	"STAT",             //01000
	"LOUT",             //01001
	"COR1",             //01010
	"COR2",             //01011
	"PWRDN_REG",        //01100
	"FLR = 13",         //01101
	"IDCODE",           //01110
	"CWDT",             //01111
	"HC_OPT_REG",       //10000
	"CSBO",             //10001
	"GENERAL1",         //10110
	"GENERAL2",         //11000
	"GENERAL3",         //11010
	"GENERAL4",
	"GENERAL5",
	"MODE_REG",
	"PU_GWE",
	"PU_GTS",
	"MFWR",
	"CCLK_FREQ",
	"SEU_OPT",
	"EXP_SIGN",
	"RDBK_SIGN",
	"BOOSTS",
	"EYE_MASK",
	"CBC_REG"
} ;

const char * sRegisterNames_V6[] = {
	"CRC",    //00000
	"FAR",    //00001
	"FDRI",   //00010
	"FDRO",   //00011
	"CMD",    //00100
	"CTL0",   //00101
	"MASK",   //00110
	"STAT",   //00111
	"LOUT",   //01000
	"COR0",   //01001
	"MFWR",   //01010
	"CBC",    //01011
	"IDCODE", //01100
	"AXSS",   //01101
	"COR1",   //01110
	"CSOB",   //01111
	"WBSTAR", //10000
	"TIMER",  //10001
	"BOOTSTS",//10110
	"CTL1",   //11000
	"DWC"     //11010
} ;

const char * sOpcodeNames[] = {
	"NOP", "READ", "WRITE"
} ;

static const INFO_t FPGAInfo[ tyLast ] = {
// Spartan-3
	{ idXC3S50    , flXC3S50    , 0, 0 },
	{ idXC3S200   , flXC3S200   , 0, 0 },
	{ idXC3S400   , flXC3S400   , 0, 0 },
	{ idXC3S1000  , flXC3S1000  , 0, 0 },
	{ idXC3S1500  , flXC3S1500  , 0, 0 },
	{ idXC3S2000  , flXC3S2000  , 0, 0 },
	{ idXC3S4000  , flXC3S4000  , 0, 0 },
	{ idXC3S5000  , flXC3S5000  , 0, 0 },

// S
	{ idXC3S100E  , flXC3S100E  ,  8, 40 },
	{ idXC3S250E  , flXC3S250E  , 20, 52 },
	{ idXC3S500E  , flXC3S500E  , 24, 88 },
	{ idXC3S1200E , flXC3S1200E , 0, flXC3S1200E },
	{ idXC3S1600E , flXC3S1600E , 0, flXC3S1600E },

//
	{ idXC3S50A   , flXC3S50A   , 0, 0 },
	{ idXC3S200A  , flXC3S200A  , 0, 0 },
	{ idXC3S400A  , flXC3S400A  , 0, 0 },
	{ idXC3S700A  , flXC3S700A  , 0, 0 },
	{ idXC3S1400A , flXC3S1400A , 0, 0 },
	{ idXC3SD1800A, flXC3SD1800A, 0, 0 },
	{ idXC3SD3400A, flXC3SD3400A, 0, 0 },

// S
	{ idXC3S50AN  , flXC3S50AN  , 0, 0 },
	{ idXC3S200AN , flXC3S200AN , 0, 0 },
	{ idXC3S400AN , flXC3S400AN , 0, 0 },
	{ idXC3S700AN , flXC3S700AN , 0, 0 },
	{ idXC3S1400AN, flXC3S1400AN, 0, 0 },

//
	{ idXC6SLX4   , idXC6SLX4   , 0, 0 },
	{ idXC6SLX9   , idXC6SLX9   , 0, 0 },
	{ idXC6SLX16  , idXC6SLX16  , 0, 0 },
	{ idXC6SLX25  , idXC6SLX25  , 0, 0 },
	{ idXC6SLX25T , idXC6SLX25T , 0, 0 },
	{ idXC6SLX45  , idXC6SLX45  , 0, 0 },
	{ idXC6SLX45T , idXC6SLX45T , 0, 0 },
	{ idXC6SLX75  , idXC6SLX75  , 0, 0 },
	{ idXC6SLX75T , idXC6SLX75T , 0, 0 },
	{ idXC6SLX100 , idXC6SLX100 , 0, 0 },
	{ idXC6SLX100T, idXC6SLX100T, 0, 0 },
	{ idXC6SLX150 , idXC6SLX150 , 0, 0 },
	{ idXC6SLX150T, idXC6SLX150T, 0, 0 },

//
	{ idXC4VLX15  , flXC4VLX15  , 0, 0 },
	{ idXC4VLX25  , flXC4VLX25  , 0, 0 },
	{ idXC4VLX40  , flXC4VLX40  , 0, 0 },
	{ idXC4VLX60  , flXC4VLX60  , 0, 0 },
	{ idXC4VLX80  , flXC4VLX80  , 0, 0 },
	{ idXC4VLX100 , flXC4VLX100 , 0, 0 },
	{ idXC4VLX160 , flXC4VLX160 , 0, 0 },

//
	{ idXC4VSX25  , flXC4VSX25  , 0, 0 },
	{ idXC4VSX35  , flXC4VSX35  , 0, 0 },
	{ idXC4VSX55  , flXC4VSX55  , 0, 0 },

//
	{ idXC4VFX12  , flXC4VFX12  , 0, 0 },
	{ idXC4VFX20  , flXC4VFX20  , 0, 0 },
	{ idXC4VFX40  , flXC4VFX40  , 0, 0 },
	{ idXC4VFX60  , flXC4VFX60  , 0, 0 },
	{ idXC4VFX100 , flXC4VFX100 , 0, 0 },
	{ idXC4VFX140 , flXC4VFX140 , 0, 0 },
	{ idXC4VLX200 , flXC4VLX200 , 0, 0 },

//
	{ idXC5VLX30  , flXC5VLX30  , 0, 0 },
	{ idXC5VLX50  , flXC5VLX50  , 0, 0 },
	{ idXC5VLX85  , flXC5VLX85  , 0, 0 },
	{ idXC5VLX110 , flXC5VLX110 , 0, 0 },
	{ idXC5VLX155 , flXC5VLX155 , 0, 0 },
	{ idXC5VLX220 , flXC5VLX220 , 0, 0 },
	{ idXC5VLX330 , flXC5VLX330 , 0, 0 },
	{ idXC5VLX20T , flXC5VLX20T , 0, 0 },
	{ idXC5VLX30T , flXC5VLX30T , 0, 0 },
	{ idXC5VLX50T , flXC5VLX50T , 0, 0 },
	{ idXC5VLX85T , flXC5VLX85T , 0, 0 },
	{ idXC5VLX110T, flXC5VLX110T, 0, 0 },
	{ idXC5VLX155T, flXC5VLX155T, 0, 0 },
	{ idXC5VLX220T, flXC5VLX220T, 0, 0 },
	{ idXC5VLX330T, flXC5VLX330T, 0, 0 },

//
	{ idXC5VSX35T , flXC5VSX35T , 0, 0 },
	{ idXC5VSX50T , flXC5VSX50T , 0, 0 },
	{ idXC5VSX95T , flXC5VSX95T , 0, 0 },
	{ idXC5VSX240T, flXC5VSX240T, 0, 0 },

//
	{ idXC5VFX30T , flXC5VFX30T , 0, 0 },
	{ idXC5VFX70T , flXC5VFX70T , 0, 0 },
	{ idXC5VFX100T, flXC5VFX100T, 0, 0 },
	{ idXC5VFX130T, flXC5VFX130T, 0, 0 },
	{ idXC5VFX200T, flXC5VFX200T, 0, 0 },
	{ idXC5VTX150T, flXC5VTX150T, 0, 0 },
	{ idXC5VTX240T, flXC5VTX240T, 0, 0 },

//
	{ idXC6VHX250T, flXC6VHX250T, 0, 0 },
	{ idXC6VHX255T, flXC6VHX255T, 0, 0 },
	{ idXC6VHX380T, flXC6VHX380T, 0, 0 },
	{ idXC6VHX565T, flXC6VHX565T, 0, 0 },

//
	{ idXC6VLX75T , flXC6VLX75T , 0, 0 },
	{ idXC6VLX130T, flXC6VLX130T, 0, 0 },
	{ idXC6VLX195T, flXC6VLX195T, 0, 0 },
	{ idXC6VLX240T, flXC6VLX240T, 0, 0 },
	{ idXC6VLX365T, flXC6VLX365T, 0, 0 },
	{ idXC6VLX550T, flXC6VLX550T, 0, 0 },
	{ idXC6VLX760 , flXC6VLX760 , 0, 0 },

//
	{ idXC6VSX315T, flXC6VSX315T, 0, 0 },
	{ idXC6VSX475T, flXC6VSX475T, 0, 0 },
	{ idXQ6VLX130T, flXQ6VLX130T, 0, 0 },
	{ idXQ6VLX240T, flXQ6VLX240T, 0, 0 },
	{ idXQ6VLX550T, flXQ6VLX550T, 0, 0 },
	{ idXQ6VSX315T, flXQ6VSX315T, 0, 0 },
	{ idXQ6VSX475T, flXQ6VSX475T, 0, 0 }
} ;


static const uint8_t InitialHeader[ 9 ] = { 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x00 } ;

// bitfile
SpartanBitfile_t bit_file ;
static size_t nLength ;
static uint8_t * pRaw ;
static uint8_t * current ;


static uint16_t swap16 ( uint16_t a ) {
	return ( a << 8 ) | ( a >> 8 ) ;
}

static uint32_t swap32 ( uint32_t a ) {
	return ( a >> 24 ) | ( ( a & 0x00ff0000 ) >> 8 ) | ( ( a & 0x0000ff00 ) << 8 ) | ( a << 24 ) ;
}

uint8_t get_byte ( void ) {
	uint8_t result = * ( uint8_t * ) current ;
	current += sizeof ( result ) ;
	return result ;
}

uint16_t get_word ( void ) {
	uint16_t result = * ( uint16_t * ) current ;
	current += sizeof ( result ) ;
	return swap16 ( result ) ;
}

uint32_t get_long ( void ) {
	uint32_t result = * ( uint32_t * ) current ;
	current += sizeof ( result ) ;
	return swap32 ( result ) ;
}

char * get_string ( void ) {
	char * result;
	uint16_t len = get_word() ;
	result = strdup ( ( char * ) current ) ;
	current += len ;
	return result ;
}

void put_byte ( uint8_t b ) {
	* ( uint8_t * ) current = b ;
	current += sizeof ( b ) ;
}

void put_word ( uint16_t b ) {
	* ( uint16_t * ) current = swap16 ( b ) ;
	current += sizeof ( b ) ;
}

void put_long ( uint32_t b ) {
	* ( uint32_t * ) current = swap32 ( b ) ;
	current += sizeof ( b ) ;
}

void put_string ( char * b ) {
	put_word ( strlen ( b ) + 1 ) ;
	while ( *b != 0 ) {
		put_byte ( *b++ ) ;
	}
	put_byte ( 0 ) ;
}

bool parse_header ( int len ) {
	bool result = true ;
	unsigned int i ;

	if ( ( * ( uint32_t * ) current ) != 0xFFFFFFFF ) {
		result &= get_word() == sizeof ( InitialHeader ) ;
		for ( i = 0 ; result && i < sizeof ( InitialHeader ) ; i += 1 ) {
			result &= InitialHeader[ i ] == get_byte() ;
		}
		if ( ! result ) {
			return result ;
		}

		result = result && get_word() == 1 ;
		result = result && get_byte() == 'a' ;
		bit_file.header.info = get_string() ;
		result = result && get_byte() == 'b' ;
		bit_file.header.part = get_string() ;
		result = result && get_byte() == 'c' ;
		bit_file.header.date = get_string() ;
		result = result && get_byte() == 'd' ;
		bit_file.header.time = get_string() ;
		result = result && get_byte() == 'e' ;
		bit_file.length = get_long() ;
		bit_file.header.bBit = true ;
	} else {
		bit_file.header.info = NULL ;
		bit_file.header.part = NULL ;
		bit_file.header.date = NULL ;
		bit_file.header.time = NULL ;
		bit_file.length = len ;
		bit_file.header.bBit = false ;
	}

	return result ;
}

bool parse_packets3 ( void ) {
	bool result = true ;
	unsigned int i, n ;
	uint32_t header, type ;
	uint32_t count, autocrc = 0 ;
	uint32_t * data ;

	n = 0 ;
	while ( current < pRaw + bit_file.length + bit_file.header_length ) {
		// get packet header
		header = get_long() ;
		type = header >> 29 ;

		// get packet length
		switch ( type ) {
		case 1:
			count = header & 0x000007ff ;
			break ;
		case 2:
			count = header & 0x07ffffff ;
			break ;
		default:
			count = 0 ;
		}
		data = NULL ;
		if ( count > 0 ) {
			data = ( uint32_t * ) malloc ( count * sizeof ( uint32_t ) ) ;
			for ( i = 0 ; i < count ; i += 1 ) {
				data[ i ] = get_long() ;
			}
		}
		if ( ( ( header >> 13 ) & 0xF ) == 2 ) { // FDRI
			autocrc = get_long() ;
		}

		bit_file.packets3 = ( Spartan3Packet_t * ) realloc ( bit_file.packets3, ( n + 1 ) * sizeof ( Spartan3Packet_t ) ) ;
		bit_file.packets3[ n ].header = header ;
		bit_file.packets3[ n ].data = data ;
		bit_file.packets3[ n ].count = count ;
		bit_file.packets6[ n ].autocrc = autocrc ;
		n += 1 ;
	}
	bit_file.count = n ;
	return result ;
}

bool parse_packets3a ( void ) {
	bool result = true ;
	unsigned int i, n ;
	uint32_t header, type ;
	uint32_t count, autocrc = 0 ;
	uint32_t * data ;

	n = 0 ;
	while ( current < pRaw + bit_file.length + bit_file.header_length ) {
		// get packet header
		header = get_long() ;
		type = header >> 29 ;

		// get packet length
		switch ( type ) {
		case 1:
			count = header & 0x000007ff ;
			break ;
		case 2:
			count = header & 0x07ffffff ;
			break ;
		default:
			count = 0 ;
		}
		data = NULL ;
		if ( count > 0 ) {
			data = ( uint32_t * ) malloc ( count * sizeof ( uint32_t ) ) ;
			for ( i = 0 ; i < count ; i += 1 ) {
				data[ i ] = get_long() ;
			}
		}
		if ( ( ( header >> 13 ) & 0xF ) == 2 ) { // FDRI
			autocrc = get_long() ;
		}

		bit_file.packets3 = ( Spartan3Packet_t * ) realloc ( bit_file.packets3, ( n + 1 ) * sizeof ( Spartan3Packet_t ) ) ;
		bit_file.packets3[ n ].header = header ;
		bit_file.packets3[ n ].data = data ;
		bit_file.packets3[ n ].count = count ;
		bit_file.packets6[ n ].autocrc = autocrc ;
		n += 1 ;
	}
	bit_file.count = n ;
	return result ;
}

bool parse_packets3e ( void ) {
	bool result = true ;
	unsigned int i, n ;
	uint32_t header, type, regnr ;
	uint32_t count, autocrc = 0 ;
	uint32_t * data ;

	n = 0 ;
	while ( current < pRaw + bit_file.length + bit_file.header_length ) {
		// get packet header
		header = get_long() ;
		type = header >> 29 ;

		// get packet length
		switch ( type ) {
		case 1:
			count = header & 0x000007ff ;
			regnr = ( header >> 13 ) & 0x3FFF ;
			break ;
		case 2:
			count = header & 0x07ffffff ;
			regnr = 2 ;
			break ;
		default:
			count = 0 ;
			regnr = 0 ;
		}
		data = NULL ;
		if ( count > 0 ) {
			data = ( uint32_t * ) malloc ( count * sizeof ( uint32_t ) ) ;
			for ( i = 0 ; i < count ; i += 1 ) {
				data[ i ] = get_long() ;
			}
			if ( regnr == 2 ) { // FDRI
				autocrc = get_long() ;
			}
		}

		bit_file.packets3 = ( Spartan3Packet_t * ) realloc ( bit_file.packets3, ( n + 1 ) * sizeof ( Spartan3Packet_t ) ) ;
		bit_file.packets3[ n ].header = header ;
		bit_file.packets3[ n ].data = data ;
		bit_file.packets3[ n ].count = count ;
		bit_file.packets6[ n ].autocrc = autocrc ;
		n += 1 ;
	}
	bit_file.count = n ;
	return result ;
}

bool parse_packets6 ( void ) {
	bool result = true ;
	unsigned int i, n ;
	uint16_t header, type ;
	uint32_t count, autocrc = 0 ;
	uint16_t * data ;

	n = 0 ;
	while ( current < pRaw + bit_file.length + bit_file.header_length ) {
		// get packet header
		header = get_word() ;
		type = header >> 13 ;

		// get packet length
		switch ( type ) {
		case 1:
			count = header & 0x001f ;
			break ;
		case 2:
			count = get_long() & 0x0fffffff ;
			break ;
		default:
			count = 0 ;
		}
		data = NULL ;
		if ( count > 0 ) {
			data = ( uint16_t * ) malloc ( count * sizeof ( uint16_t ) ) ;
			for ( i = 0 ; i < count ; i += 1 ) {
				data[ i ] = get_word() ;
			}
		}
		if ( type == 2 )
			autocrc = get_long() ;

		bit_file.packets6 = ( Spartan6Packet_t * ) realloc ( bit_file.packets6, ( n + 1 ) * sizeof ( Spartan6Packet_t ) ) ;
		bit_file.packets6[ n ].header = header ;
		bit_file.packets6[ n ].data = data ;
		bit_file.packets6[ n ].count = count ;
		bit_file.packets6[ n ].autocrc = autocrc ;
		n += 1 ;
	}
	bit_file.count = n ;
	return result ;
}

#define CRC_POLY 0x1EDC6F41
#define CRC_SIZE 32
#define CRC_MASK 0xFFFFFFFF

//#define CRC_POLY 0x5d6dcb
//#define CRC_POLY 0x864cfb
//#define CRC_POLY 0xbe64c3
//#define CRC_SIZE 24
//#define CRC_MASK 0xFFFFFF


static uint32_t crc = 0 ;

static uint32_t reflect ( uint32_t val, int bits ) {
	uint32_t vers ;

	vers = 0 ;
	while ( bits-- ) {
		vers = ( vers << 1 ) | ( val & 1 ) ;
		val >>= 1 ;
	}
	return vers ;
}


static void one_bit ( uint32_t data ) {
	uint32_t val = ( ( crc >> ( CRC_SIZE - 1 ) ) ^ data ) & 1 ;

	crc <<= 1 ;
	crc &= CRC_MASK ;
	if ( val != 0 )
		crc ^= CRC_POLY ;
}


static void one_word ( uint16_t word, uint8_t reg ) {
	int i ;

    uint32_t cat = ( reg << 16 ) | word ;

	for ( i = 0 ; i < 16 + 6 ; i += 1 )
		one_bit ( cat >> i ) ;
}


void show_file ( void ) {
	int i, j, k ;
	FILE * outfile ;

	outfile = fopen ( "log.txt", "w" ) ;
	if ( outfile == NULL ) {
		fprintf( stderr, "? Unable to open or create output dump file '%s'\n", "log.txt" ) ;
		return ;
	}

	if ( bit_file.header.bBit ) {
		fprintf( outfile, "! header:\n" ) ;
		fprintf( outfile, "! info: %s\n", bit_file.header.info ) ;
		fprintf( outfile, "! part: %s\n", bit_file.header.part ) ;
		fprintf( outfile, "! date: %s\n", bit_file.header.date ) ;
		fprintf( outfile, "! time: %s\n", bit_file.header.time ) ;
	}

	fprintf( outfile, "! number of packets: %d\n", bit_file.count ) ;
	for ( i = 0 ; i < ( int ) bit_file.count ; i += 1 ) {
		switch ( bit_file.type ) {
		case bstSpartan3:
			fprintf( outfile, "! %4d: 0x%08X", i, bit_file.packets3[ i ].header ) ;
			fprintf( outfile, " %5s", sOpcodeNames[ ( bit_file.packets3[ i ].header >> 27 ) & 0x0003 ] ) ;
			if ( ( ( bit_file.packets3[ i ].header >> 27 ) & 0x0003 ) != 0 ) {
				if ( ( ( bit_file.packets3[ i ].header >> 29 ) & 0x0003 ) == 1 ) {
					fprintf( outfile, " %6s," , sRegisterNames_S3[ ( bit_file.packets3[ i ].header >> 13 ) & 0x003FFF ] ) ;
				} else {
					fprintf( outfile, "            " ) ;
				}
				fprintf( outfile, " count: %6d : ", bit_file.packets3[ i ].count ) ;
			}
			if ( bit_file.packets3[ i ].count > 0 )
				for ( j = 0 ; j < 37 && j < ( int ) bit_file.packets3[ i ].count ; j += 1 ) {
					fprintf( outfile, "%08X", bit_file.packets3[ i ].data[ j ] ) ;
				}
			if ( ( ( bit_file.packets3[ i ].header >> 13 ) & 0xF ) == 2 ) {
				fprintf( outfile, " : autocrc: 0x%08X\n", bit_file.packets3[ i ].autocrc ) ;
			} else if ( ( ( bit_file.packets3[ i ].header >> 13 ) & 0xF ) == 8 ) {
				int n = bit_file.packets3[ i ].data[ 0 ] ; // XAPP452.pdf, v1.1 : Figure 2
				fprintf( outfile, " : block %d major %d minor %d\n", ( n >> 25 ) & 0x3, ( n >> 17 ) & 0xFF, ( n >> 9 ) & 0xFF ) ;
			} else {
				fprintf( outfile, "\n" ) ;
			}
			break;

		case bstSpartan3a:
			fprintf( outfile, "! %4d: 0x%08X", i, bit_file.packets3[ i ].header ) ;
			fprintf( outfile, " %5s", sOpcodeNames[ ( bit_file.packets3[ i ].header >> 27 ) & 0x0003 ] ) ;
			if ( ( ( bit_file.packets3[ i ].header >> 27 ) & 0x0003 ) != 0 ) {
				if ( ( ( bit_file.packets3[ i ].header >> 29 ) & 0x0003 ) == 1 ) {
					fprintf( outfile, " %6s," , sRegisterNames_S3[ ( bit_file.packets3[ i ].header >> 13 ) & 0x003FFF ] ) ;
				} else {
					fprintf( outfile, "            " ) ;
				}
				fprintf( outfile, " count: %6d : ", bit_file.packets3[ i ].count ) ;
			}
			if ( bit_file.packets3[ i ].count > 0 )
				for ( j = 0 ; j < 37 && j < ( int ) bit_file.packets3[ i ].count ; j += 1 ) {
					fprintf( outfile, "%08X", bit_file.packets3[ i ].data[ j ] ) ;
				}
			if ( ( ( bit_file.packets3[ i ].header >> 13 ) & 0xF ) == 2 ) {
				fprintf( outfile, " : autocrc: 0x%08X\n", bit_file.packets3[ i ].autocrc ) ;
			} else if ( ( ( bit_file.packets3[ i ].header >> 13 ) & 0xF ) == 8 ) {
				int n = bit_file.packets3[ i ].data[ 0 ] ; // XAPP452.pdf, v1.1 : Figure 2
				fprintf( outfile, " : block %d major %d minor %d\n", ( n >> 25 ) & 0x3, ( n >> 17 ) & 0xFF, ( n >> 9 ) & 0xFF ) ;
			} else {
				fprintf( outfile, "\n" ) ;
			}
			break;

		case bstSpartan3e: {
			FPGAType_e FPGAType = tyXC3S500E ;

			fprintf( outfile, "! %4d: 0x%08X", i, bit_file.packets3[ i ].header ) ;
			fprintf( outfile, " %5s", sOpcodeNames[ ( bit_file.packets3[ i ].header >> 27 ) & 0x0003 ] ) ;
			if ( ( ( bit_file.packets3[ i ].header >> 27 ) & 0x0003 ) != 0 ) {
				if ( ( ( bit_file.packets3[ i ].header >> 29 ) & 0x0003 ) == 1 ) {
					fprintf( outfile, " %6s," , sRegisterNames_S3[ ( bit_file.packets3[ i ].header >> 13 ) & 0x003FFF ] ) ;
				} else {
					fprintf( outfile, "            " ) ;
				}
				fprintf( outfile, " count: %6d : ", bit_file.packets3[ i ].count ) ;
			}
			if ( bit_file.packets3[ i ].count > 0 ) {
				int col = 0, coldef = 0, blk = 0, maj = 0, min = 0 ;
				coldef = abs ( ColumnDefs[FPGAType][ col ] ) ;

				for ( k = 0 ; k < ( int ) bit_file.packets3[ i ].count ; ) {
					if ( ( ( bit_file.packets3[ i ].header >> 29 ) & 0x0003 ) == 2 ) { // FDRI type 2
						fprintf( outfile, "\nblk %2d, maj %2d, min %2d : ", blk, maj, min ) ;
					} else if ( ( ( bit_file.packets3[ i ].header >> 13 ) & 0x3fff ) == 2 ) { // FDRI type 1
						fprintf( outfile, "blk %2d, maj %2d, min %2d : ", blk, maj, min ) ;
					} else if ( ( ( bit_file.packets3[ i ].header >> 13 ) & 0x3fff ) == 8 ) { // LOUT debugging
						int n = bit_file.packets3[ i ].data[ k ] ; // XAPP452.pdf, v1.1 : Figure 2
						fprintf( outfile, " - blk %2d, maj %2d, min %2d : ", ( n >> 25 ) & 0x3, ( n >> 17 ) & 0xFF, ( n >> 9 ) & 0xFF ) ;
					}
					// 12:00008102 13:04081020 14:40810204 15:08102040 16:81002040 17:81020408 18:10204081 19:02040810 20:20400000
					// 80:00008102 81:04081020 82:40810204 83:08102040 84:81002040 85:81020408 86:10204081 87:02040810 88:20400000

					//  8:00008102  9:04081020 10:40810204 11:08102040 12:81002040 13:81020408 14:10204081 15:02040810 16:20408102
					// 17:04081020 18:40810204 19:08102040 20:81002040 21:81020408 22:10204081 23:02040810 24:20408102 25:04081020
					// 26:40810204 27:08102040 28:81002040 29:81020408 30:10204081 31:02040810 32:20408102 33:04081020 34:40810204
					// 35:08102040 36:81002040 37:81020408 38:10204081 39:02040810 40:20400000

					for ( j = 0 ; j < FPGAInfo[ FPGAType ].fl && k < ( int ) bit_file.packets3[ i ].count ; j += 1, k += 1 ) {
//                       if ( 0 <= j && j <= FPGAInfo[ FPGAType ].fl )
						if ( FPGAInfo[ FPGAType ].bs <= j && j <= FPGAInfo[ FPGAType ].be ) {
							fprintf( outfile, "%2d:%08X ", j, bit_file.packets3[ i ].data[ k ] ) ;
							if ( maj == 1 )
								fwrite ( &bit_file.packets3[ i ].data[ k ], sizeof ( uint32_t ), 1, outfile ) ;
						}
					}
					min += 1 ;
					if ( min >= coldef ) {
						fprintf( outfile, "\n" ) ;
						min = 0 ;
						col += 1 ;
						coldef = abs ( ColumnDefs[ FPGAType ][ col ] ) ;
						if ( ColumnDefs[ FPGAType ][ col ] < 0 ) {
							maj = 0 ;
							blk += 1 ;
						} else {
							maj += 1 ;
						}
					}
				}
			}
			if ( ( bit_file.packets3[ i ].header >> 29 ) == 1 ) {
				fprintf( outfile, "\n" ) ;
			} else if ( ( bit_file.packets3[ i ].header >> 29 ) == 2 ) {
				fprintf( outfile, " : autocrc: 0x%08X\n", bit_file.packets3[ i ].autocrc ) ;
			} else {
				fprintf( outfile, "\n" ) ;
			}
		}
		break;

		case bstSpartan6:
			fprintf( outfile, "! %4d: 0x%04X", i, bit_file.packets6[ i ].header ) ;
			fprintf( outfile, " %6s " , sOpcodeNames[ ( bit_file.packets6[ i ].header >> 11 ) & 0x0003 ] ) ;
			if ( ( ( bit_file.packets6[ i ].header >> 11 ) & 0x0003 ) != 0 ) {
				fprintf( outfile, " %8s," , sRegisterNames_S6[ ( bit_file.packets6[ i ].header >> 5 ) & 0x003F ] ) ;
				fprintf( outfile, "  count:  %d", bit_file.packets6[ i ].count ) ;
				switch ( bit_file.packets6[ i ].header >> 13 ) {
				case 1 :
					switch ( ( bit_file.packets6[ i ].header >> 5 ) & 0x003F ) {
                    case 0 :
                        fprintf( outfile, " crc = %06X\n", crc ) ;
                        break;
					}
                    for ( j = 0, k = 0 ; j < ( int ) bit_file.packets6[ i ].count ; j += 1, k = ( k + 1 ) & 31 ) {
                        if ( k == 0 )
                            fprintf( outfile, "\n" ) ;
                        fprintf( outfile, "%04X ", bit_file.packets6[ i ].data[ j ] ) ;
                        one_word ( bit_file.packets6[ i ].data[ j ], ( bit_file.packets6[ i ].header >> 5 ) & 0x003F ) ;
                    }
					switch ( ( bit_file.packets6[ i ].header >> 5 ) & 0x003F ) {
                    case 0 :
                        fprintf( outfile, " crc = %06X\n", crc ) ;
                        break;
					case 5 :
						switch ( bit_file.packets6[ i ].data[ 0 ] ) {
                        case 7 :
							crc = 0 ;
							fprintf( outfile, ": crc cleared" ) ;
                            fprintf( outfile, " crc = %06X\n", crc ) ;
							break ;
						}
                        break ;
                    case 13 :
                        fprintf( outfile, ": frame length = %d", bit_file.packets6[ i ].data[ 0 ] ) ;
                        break ;
					}
					break ;
				case 2 :
					for ( j = 0, k = 0 ; j < ( int ) bit_file.packets6[ i ].count ; j += 1, k = ( k + 1 ) & 31 ) {
						if ( k == 0 )
							fprintf( outfile, "\n" ) ;
						fprintf( outfile, "%04X ", bit_file.packets6[ i ].data[ j ] ) ;
						one_word ( bit_file.packets6[ i ].data[ j ], ( bit_file.packets6[ i ].header >> 5 ) & 0x003F ) ;
					}
					fprintf( outfile, "\n" ) ;

					fprintf( outfile, "%06X\n", crc ) ;
					fprintf( outfile, "%06X\n", bit_file.packets6[ i ].autocrc ) ;
					fprintf( outfile, "%06X\n", reflect( bit_file.packets6[ i ].autocrc, 24 ) ) ;

//                        one_word( bit_file.packets6[ i ].autocrc >> 16, ( bit_file.packets6[ i ].header >> 5 ) & 0x003F ) ;
//                        one_word( bit_file.packets6[ i ].autocrc & 0xFFFF, ( bit_file.packets6[ i ].header >> 5 ) & 0x003F ) ;

					break ;
				}
			}
			fprintf( outfile, "\n" ) ;
			break;
		}
	}
	fclose ( outfile ) ;
}

bool merge_code ( uint16_t * code, int len, int nr, int bVerbose ) {
	int i, j, s ;
	uint16_t data, * packet, * p ;
	int state ;

	// find bulk frame

	for ( i = 0 ; i < ( int ) bit_file.count ; i += 1 ) {
		if ( bit_file.packets6[ i ].header == 0x5060 ) {
			if ( bVerbose > 0 ) {
				printf ( "! using bulk packet# %d\n", i ) ;
			}
			state = -1 ;
			for ( j = 0 ; j < ( int ) bit_file.packets6[ i ].count ; j += 1 ) {
				data = bit_file.packets6[ i ].data[ j ] ;
				if ( state == -1 ) {
					s = j ;
				}
				switch ( data ) {
					/* 0 */
				case 0xf01c :
					if ( nr == 0 ) {
						state = 0 ;
					}
					break ;
				case 0xcaf4 :
					if ( nr == 1 ) {
						state = 0 ;
					}
					break ;
				case 0xc90b :
					if ( nr == 2 ) {
						state = 0 ;
					}
					break ;
				case 0xc4b3 :
					if ( nr == 3 ) {
						state = 0 ;
					}
					break ;
				case 0xe030 :
					if ( nr == 4 ) {
						state = 0 ;
					}
					break ;
				case 0xe78d :
					if ( nr == 5 ) {
						state = 0 ;
					}
					break ;
				case 0xf199 :
					if ( nr == 6 ) {
						state = 0 ;
					}
					break ;
				case 0xe481 :
					if ( nr == 7 ) {
						state = 0 ;
					}
					break ;
				case 0xcac2 :
					if ( nr == 8 ) {
						state = 0 ;
					}
					break ;
				case 0xc991 :
					if ( nr == 9 ) {
						state = 0 ;
					}
					break ;
				case 0xe1eb :
					if ( nr == 10 ) {
						state = 0 ;
					}
					break ;
				case 0xd3ec :
					if ( nr == 11 ) {
						state = 0 ;
					}
					break ;
				case 0xefa9 :
					if ( nr == 12 ) {
						state = 0 ;
					}
					break ;
				case 0xe83b :
					if ( nr == 13 ) {
						state = 0 ;
					}
					break ;
				case 0xe5dc :
					if ( nr == 14 ) {
						state = 0 ;
					}
					break ;
				case 0xd366 :
					if ( nr == 15 ) {
						state = 0 ;
					}
					break ;
					/* 1 */
				case 0xffa7 :
				case 0xf74a :
				case 0x7c90 :
				case 0xb7f9 :
				case 0x71fc :
				case 0x3e18 :
				case 0xf21e :
				case 0x7f59 :
				case 0x7666 :
				case 0xb961 :
				case 0xfbde :
				case 0xf81b :
				case 0x7c4a :
				case 0x78df :
				case 0xf0ab :
				case 0xbfda :
					if ( state == 0 ) {
						state = 1 ;
					}
					break ;
					/* 2 */
				case 0x2c05 :
				case 0x5c6c :
				case 0x9c00 :
				case 0x5cca :
				case 0xaf07 :
				case 0x3f0b :
				case 0x6c28 :
				case 0x0f37 :
				case 0xdc92 :
				case 0x9e31 :
				case 0xcfa1 :
				case 0xce1d :
				case 0xde18 :
				case 0xbce8 :
				case 0x1cb4 :
				case 0xafc9 :
					if ( state == 1 ) {
						state = 2 ;
					}
					break ;
					/* 3 */
				case 0xaf1f :
				case 0x9b04 :
				case 0xa7b9 :
				case 0xcbf8 :
				case 0x8feb :
				case 0x0329 :
				case 0xe3ab :
				case 0x1f7d :
				case 0x6b0c :
				case 0x8741 :
				case 0xbf49 :
				case 0x67af :
				case 0x97fe :
				case 0x9f3f :
				case 0xd767 :
				case 0x3f25 :
					if ( state == 2 ) {
						state = 3 ;
					}
					break ;
					/* 4 */
				case 0x1aca :
				case 0x6df8 :
				case 0xe8d4 :
				case 0xddc5 :
				case 0x98f6 :
				case 0xdaec :
				case 0x7bc1 :
				case 0x9fc4 :
				case 0x67c4 :
				case 0xf8f5 :
				case 0x12ce :
				case 0x91e8 :
				case 0x65d0 :
				case 0xd2e8 :
				case 0xeafc :
				case 0x13e4 :
					if ( state == 3 ) {
						state = 4 ;
					}
					break ;
					/* 5 */
				case 0x1cf5 :
				case 0x05ff :
				case 0x7832 :
				case 0x26b7 :
				case 0x3b38 :
				case 0x9af6 :
				case 0xf0be :
				case 0x3abf :
				case 0xba31 :
				case 0x9c35 :
				case 0x0c72 :
				case 0x8bf1 :
				case 0x3b35 :
				case 0x4bfb :
				case 0xe7ba :
				case 0x04b8 :
					if ( state == 4 ) {
						state = 5 ;
					}
					break ;
					/* 6 */
				case 0x228d :
				case 0x272e :
				case 0xc23d :
				case 0xe88f :
				case 0xe81d :
				case 0xb56c :
				case 0xef3f :
				case 0x41ec :
				case 0x4a2f :
				case 0x94bc :
				case 0x7d3e :
				case 0xe37c :
				case 0x4abf :
				case 0x546c :
				case 0x8dec :
				case 0xeb9d :
					if ( state == 5 ) {
						state = 6 ;
					}
					break ;
					/* 7 */
				case 0x1ef3 :
				case 0xf8f3 :
				case 0x01bb :
				case 0x9f3b :
				case 0x950f :
				case 0xc777 :
				case 0x1387 :
				case 0x61f7 :
				case 0xd39b :
				case 0x17d7 :
				case 0xccdf :
				case 0x5ff7 :
				case 0xf167 :
				case 0x9b43 :
				case 0x32e7 :
				case 0x472b :
					if ( state == 6 ) {
						state = 7 ;
					}
					break ;
					/* 8 */
				case 0xb4b1 :
				case 0x96a3 :
				case 0xa2ef :
				case 0xe45e :
				case 0x7d06 :
				case 0x751d :
				case 0xfaea :
				case 0xd72d :
				case 0xfad6 :
				case 0x0a80 :
				case 0xe99b :
				case 0x3470 :
				case 0x0fa2 :
				case 0x23a3 :
				case 0xe50c :
				case 0x7dff :
					if ( state == 7 ) {
						packet = &bit_file.packets6[ i ].data[ s ] ;
						// check whether the rest of the block is zero
						for ( i = 0, data = 0, p = packet + 9 ; i < len * 18 / 16 - 9 ; i += 1 ) {
							data |= *p++ ;
						}
						if ( data != 0 ) {
							fprintf ( stderr, "? Could not find correctly pre-initialized blockram nr: '%d' in bitfile\n", nr ) ;
							return false ;
						}

						if ( bVerbose ) {
							printf ( "! found pre-initialized blockram nr: '%d' at offset: %d\n", nr, s ) ;
						}
						memcpy ( packet, code, sizeof ( uint16_t ) * len * 18 / 16 ) ;
						state = -1 ;
						return true ;
					}
					break ;
				default :
					state = -1 ;
				}
			}
		}
	}
	fprintf ( stderr, "? Could not find pre-initialized blockram nr: '%d' in bitfile\n", nr ) ;
	return false ;
}

bool get_code ( uint16_t * code, uint32_t * p, int len ) {
	int i, j, s ;
	uint16_t data ;
	int state ;

	// find bulk frame
	for ( i = 0 ; i < ( int ) bit_file.count ; i += 1 ) {
		if ( bit_file.packets6[ i ].header == 0x5060 ) {
			state = 0 ;
			for ( j = 0 ; j < ( int ) bit_file.packets6[ i ].count ; j += 1 ) {
				if ( state == 0 ) {
					s = j ;
				}
				data = bit_file.packets6[ i ].data[ j ] ;
				if ( data == p[ state ] ) {
					state += 1 ;
				} else {
					state = 0 ;
				}
				if ( state >= len ) {
					break ;
				}
			}
			if ( state > 0 ) {
				for ( j = 0 ; j < 1024 * 18 / 16 ; j += 1, s += 1 ) {
					code[ j ] = bit_file.packets6[ i ].data[ s ] ;
				}
				return true ;
			}
		}
	}
	return false ;
}

void build_header ( void ) {
	// build bitfile header
	int i ;

	// not for BIN files
	if ( bit_file.header.bBit ) {
		put_word ( sizeof ( InitialHeader ) ) ;
		for ( i = 0 ; i < ( int ) sizeof ( InitialHeader ) ; i += 1 ) {
			put_byte ( InitialHeader[ i ] ) ;
		}
		put_word ( 1 ) ;
		put_byte ( 'a' ) ;
		put_string ( bit_file.header.info ) ;
		put_byte ( 'b' ) ;
		put_string ( bit_file.header.part ) ;
		put_byte ( 'c' ) ;
		put_string ( bit_file.header.date ) ;
		put_byte ( 'd' ) ;
		put_string ( bit_file.header.time ) ;
		put_byte ( 'e' ) ;
		put_long ( bit_file.length ) ;
	}
}

void build_sync ( void ) {
	// build a sync frame
	put_long ( 0xffffffff ) ;
	put_long ( 0xffffffff ) ;
	put_long ( 0xffffffff ) ;
	put_long ( 0xffffffff ) ;
	put_long ( 0xaa995566 ) ;
}

void build_packets ( void ) {
	int i, j ;

	// dump all headers with their data
	for ( i = 0 ; i < ( int ) bit_file.count ; i += 1 ) {
		put_word ( bit_file.packets6[ i ].header ) ;
		if ( bit_file.packets6[ i ].header >> 13 == 2 ) {
			put_long ( bit_file.packets6[ i ].count ) ;
		}
		for ( j = 0 ; j < ( int ) bit_file.packets6[ i ].count ; j += 1 ) {
			put_word ( bit_file.packets6[ i ].data[ j ] ) ;
		}
		if ( bit_file.packets6[ i ].header >> 13 == 2 ) {
			put_long ( bit_file.packets6[ i ].autocrc ) ;
		}
	}
}

bool parse_file ( const char * strBitfile, BitStreamType_e bsType, int bVerbose ) {
	bool result = true ;
	size_t nSize ;
	uint32_t sync ;
	FILE * infile = NULL ;

	infile = fopen ( strBitfile, "rb" ) ;
	if ( infile == NULL ) {
		fprintf ( stderr, "? Unable to open source bitstream file '%s'\n", strBitfile ) ;
		return false ;
	}

	// obtain file size:
	fseek ( infile , 0, SEEK_END ) ;
	nSize = ftell ( infile ) ;
	rewind ( infile ) ;

	// allocate memory to contain the whole file:
	pRaw = ( uint8_t * ) malloc ( sizeof ( char ) * nSize ) ;
	if ( pRaw == NULL ) {
		fprintf ( stderr, "? Unable to allocate buffer space\n" ) ;
		goto _close;
	}

	// copy the file into the buffer:
	nLength = fread ( pRaw, sizeof ( char ), nSize, infile ) ;
	if ( nLength != nSize ) {
		fprintf ( stderr, "? Problem reading bitstream file\n" ) ;
		goto _free;
	}

	// initialize parser
	current = pRaw ;

	// parse header
	result &= parse_header ( nLength ) ;

	// rest of file
	bit_file.header_length = current - pRaw ;

	bit_file.type = bsType ;
	bit_file.packets3 = NULL ;
	bit_file.packets6 = NULL ;

	// sync words should follow
	switch ( bsType ) {
	case bstSpartan6:
	case bstSpartan3 :
	case bstSpartan3e :
		while ( result ) {
			sync = get_long() ;
			if ( sync == 0xFFFFFFFF )
				;
			else if ( sync == 0xAA995566 ) {
				break ;
			} else {
				fprintf ( stderr, "? sync word not found\n" ) ;
				goto _free ;
			}
		}
		break ;
	case bstSpartan3a :
		while ( result ) {
			sync = get_word() ;
			if ( sync == 0xFFFF )
				;
			else if ( sync == 0xAA99 ) {
				break ;
			} else {
				fprintf ( stderr, "? sync word not found\n" ) ;
				goto _free ;
			}
		}
		break ;
	}

	// the actual work
	switch ( bsType ) {
	case bstSpartan3:
		result = result && parse_packets3() ;
		break;
	case bstSpartan3a:
		result = result && parse_packets3a() ;
		break;
	case bstSpartan3e:
		result = result && parse_packets3e() ;
		break;
	case bstSpartan6:
		result = result && parse_packets6() ;
		break;
	}

// report
	if ( result && ( bVerbose > 1 ) )
		show_file() ;

	free ( pRaw ) ;
	fclose ( infile ) ;

	return result ;

_free:
	free ( pRaw ) ;

_close:
	fclose ( infile ) ;
	return false ;
}

bool write_file ( const char * strBitfile ) {
	size_t nSize ;
	FILE * outfile ;

	outfile = fopen ( strBitfile, "wb" ) ;
	if ( outfile == NULL ) {
		fprintf ( stderr, "? Unable to open or create output bitstream file '%s'\n", strBitfile ) ;
		return false ;
	}
	pRaw = ( uint8_t * ) calloc ( bit_file.length + bit_file.header_length, sizeof ( char ) ) ;

	// initialize parser
	current = pRaw ;

	build_header() ;
	build_sync() ;
	build_packets() ;

	nSize = fwrite ( pRaw, sizeof ( char ), bit_file.length + bit_file.header_length, outfile ) ;

	fflush ( outfile ) ;
	free ( pRaw ) ;
	fclose ( outfile ) ;

	return nSize == bit_file.length + bit_file.header_length ;
}




