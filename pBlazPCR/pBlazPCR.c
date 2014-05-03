/*
 *  Copyright © 2003..2013 : Henk van Kampen <henk@mediatronix.com>
 *
 *  This file is part of pBlazASM.
 *
 *  pBlazPCR is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pBlazASM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pBlazASM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#include "pbLibgen.h"
#include "pbPicoCore.h"
#include "version.h"

#if defined TCC || defined _MSC_VER
#include "getopt.h"
#else
#include <unistd.h>
#endif

// data
static INST_t Code[ MAXMEM ] ;
static uint32_t Data[ MAXSCR ] ;
static uint64_t inst_map = 0ll ;
static int code_size = 0 ;
static int stack_size = 0 ;
static int pad_size = 0 ;
static int bank_size = 0 ;
static bool want_alu = false ;


static void usage ( const char * text ) {
	printf ( "\n%s - PicoCore Builder V%ld.%ld.%ld (%s)\n", text, MAJOR, MINOR, BUILDS_COUNT, STATUS ) ;

    printf ( "\nThis program comes with ABSOLUTELY NO WARRANTY.\n"  ) ;
    printf ( "This is free software, and you are welcome to redistribute it\n"  ) ;
    printf ( "under certain conditions. See <http://www.gnu.org/licenses/>\n"  ) ;

	printf ( "\nUsage:\n" ) ;
	printf ( "   pBlazPCR [-v] -c<MEM code inputfile> -s<MEM data inputfile> <VHD outputfile>\n" ) ;
	printf ( "   where:\n"
	         "         -c      loads a code MEM file\n"
	         "         -s      loads a data MEM file\n"
	         "         -v      generates verbose reporting\n"
	       ) ;
}

bool loadMEM ( const char * strMEMfile ) {
	int addr ;
	uint32_t code ;
	char line[ 256 ], *p ;
	FILE * infile = NULL ;

	if ( strMEMfile == NULL || * strMEMfile == 0 ) {
		return true ;
	}

	infile = fopen ( strMEMfile, "r" ) ;
	if ( infile == NULL ) {
        fprintf ( stderr, "? Unable to open MEM file '%s'\n", strMEMfile ) ;
		return false ;
	}

	for ( addr = -1 ; addr < MAXMEM && fgets ( line, sizeof ( line ), infile ) != NULL; ) {
		if ( ( p = strchr ( line, '@' ) ) != NULL ) {
			if ( sscanf ( ++p, "%X", &addr ) != 1 ) {
                fprintf ( stderr, "? Error in address in MEM file '%s'\n", strMEMfile ) ;
				return false ;
			}
		} else {
			if ( addr == -1 ) {
				fprintf ( stderr, "? Missing address in MEM file '%s', assuming 0\n", strMEMfile ) ;
				addr = 0 ;
				// return false ;
			}
			sscanf ( line, "%X", &code ) ;
			Code[ addr ].addr = addr ;
			Code[ addr ].code = code ;
			Code[ addr ].blank = false ;

			if ( code > 0 && addr >= code_size )
				code_size = addr + 1 ;

			addr += 1 ;
		}
	}

	fclose ( infile ) ;
	return true ;
}

bool loadSCR ( const char * strDatafile, const int offset ) {
	int addr ;
	uint32_t data ;
	char line[ 256 ], *p ;
	FILE * infile = NULL ;
    (void)offset ;

	if ( strDatafile == NULL || * strDatafile == 0 )
		return true ;

	infile = fopen ( strDatafile, "r" ) ;
	if ( infile == NULL ) {
		fprintf ( stderr, "? Unable to open data SCR file '%s'\n", strDatafile ) ;
		return false ;
	}

	for ( addr = -1 ; fgets ( line, sizeof ( line ), infile ) != NULL; ) {
		if ( ( p = strchr ( line, '@' ) ) != NULL ) {
			if ( sscanf ( ++p, "%X", &addr ) != 1 ) {
				fprintf ( stderr, "? Bad address in data SCR file '%s'\n", strDatafile ) ;
				return false ;
			}
		} else {
			if ( addr == -1 ) {
				fprintf ( stderr, "? Missing address in data SCR file '%s'\n", strDatafile ) ;
				return false ;
			}
			sscanf ( line, "%X", &data ) ;
			Data[ addr & ( MAXSCR - 1 ) ] = data ;
			addr += 1 ;
		}
	}
	pad_size = 256 ;

	fclose ( infile ) ;
	return true ;
}

static int stack_count = 0 ;
void checkPB6 ( int pc ) {
	uint32_t code ;

	if ( pc == 0 )
        stack_count = 0 ;

	Code[ pc ].label = true ;
	while ( pc < MAXMEM ) {
		code = Code[ pc ].code & 0x3FFFF ;
		inst_map |= 1LLU << ( code >> 12 ) ;

		if ( Code[ pc ].breadcrum )
			return ;

		switch ( code ) {
// OPs
		case 0x00000 ... 0x00FFF :
		case 0x01000 ... 0x01FFF :
		case 0x16000 ... 0x16FFF :

		case 0x02000 ... 0x02FFF :
		case 0x03000 ... 0x03FFF :
		case 0x04000 ... 0x04FFF :
		case 0x05000 ... 0x05FFF :
		case 0x06000 ... 0x06FFF :
		case 0x07000 ... 0x07FFF :
		case 0x0C000 ... 0x0CFFF :
		case 0x0D000 ... 0x0DFFF :
		case 0x0E000 ... 0x0EFFF :
		case 0x0F000 ... 0x0FFFF :
		case 0x10000 ... 0x10FFF :
		case 0x11000 ... 0x11FFF :
		case 0x12000 ... 0x12FFF :
		case 0x13000 ... 0x13FFF :
		case 0x18000 ... 0x18FFF :
		case 0x19000 ... 0x19FFF :
		case 0x1A000 ... 0x1AFFF :
		case 0x1B000 ... 0x1BFFF :
		case 0x1C000 ... 0x1CFFF :
		case 0x1D000 ... 0x1DFFF :
		case 0x1E000 ... 0x1EFFF :
		case 0x1F000 ... 0x1FFFF :
			want_alu = true ;

		case 0x2E000 ... 0x2EFFF :
		case 0x2F000 ... 0x2FFFF :
		case 0x0A000 ... 0x0AFFF :
		case 0x0B000 ... 0x0BFFF :

		case 0x2C000 ... 0x2CFFF :
		case 0x2D000 ... 0x2DFFF :
		case 0x2B000 ... 0x2BFFF :
		case 0x08000 ... 0x08FFF :
		case 0x09000 ... 0x09FFF :
			if ( bank_size <= (int)DestReg ( code ) )
				bank_size = DestReg ( code ) + 1 ;

			Code[ pc ].breadcrum = true ;
			pc += 1 ;
			break ;

		case 0x28000 ... 0x28001 :
		case 0x37000 ... 0x37001 :
			Code[ pc ].breadcrum = true ;
			pc += 1 ;
			break ;

// RR, SR, RL, SL
		case 0x14000 ... 0x14FFF :
			switch ( code & 0xFF ) {
			case 0x80 :

			case 0x02 :
			case 0x06 :
			case 0x07 :
			case 0x00 :
			case 0x04 :

			case 0x0C :
			case 0x0E :
			case 0x0F :
			case 0x08 :
			case 0x0A :
				if ( bank_size <= (int)DestReg ( code ) )
					bank_size = DestReg ( code ) + 1 ;

				Code[ pc ].breadcrum = true ;
				pc += 1 ;
				break ;
            default:
                return ;
			}
			break ;

// JUMP
		case 0x22000 ... 0x22FFF :
			Code[ pc ].breadcrum = true ;
			checkPB6 ( Address12 ( code ) ) ;
			break ;

// JUMP cond , CALL, CALL cond
		case 0x20000 ... 0x20FFF :
		case 0x30000 ... 0x30FFF :
		case 0x34000 ... 0x34FFF :
		case 0x38000 ... 0x38FFF :
		case 0x3C000 ... 0x3CFFF :
		case 0x24000 ... 0x24FFF :
			stack_count += 1 ;
			Code[ pc ].breadcrum = true ;
			checkPB6 ( Address12 ( code ) ) ;
			if ( stack_size < stack_count )
                stack_size = stack_count ;
			stack_count -= 1 ;
			pc += 1 ;
			break ;

		case 0x32000 ... 0x32FFF :
		case 0x36000 ... 0x36FFF :
		case 0x3A000 ... 0x3AFFF :
		case 0x3E000 ... 0x3EFFF :
		case 0x26000 ... 0x26FFF :
			Code[ pc ].breadcrum = true ;
			checkPB6 ( Address12 ( code ) ) ;
			pc += 1 ;
			break ;

// RET sX, KK
		case 0x25000 ... 0x25FFF :
			if ( bank_size <= (int)DestReg ( code ) )
				bank_size = DestReg ( code ) + 1 ;
// RET, RETI
		case 0x29000 ... 0x29001 :
			Code[ pc ].breadcrum = true ;
			return ;

// RET cond
		case 0x31000 ... 0x31FFF :
		case 0x35000 ... 0x35FFF :
		case 0x39000 ... 0x39FFF :
		case 0x3D000 ... 0x3DFFF :
		case 0x21000 ... 0x21FFF :
			Code[ pc ].breadcrum = true ;
			pc += 1 ;
			checkPB6 ( pc ) ;
			return ;

		default :
			return ;
		}
	}
}

int main ( int argc, char * argv[] ) {
	char * code_filename = NULL ;
	char * data_filename = NULL ;
	char * vhd_filename = NULL ;

	bool result = false ;
	bool bOptErr = false ;
	bool bVerbose = false ;
	bool bMEM = false ;
	bool bSCR = false ;

	extern char * optarg ;
	extern int optind, optopt, opterr ;
	int i, optch ;

	opterr = -1 ;
	while ( ( optch = getopt ( argc, argv, "c:d:hm:s:v" ) ) != -1 ) {
		switch ( optch ) {
		case 'c' :
		case 'm' :
			bMEM = true ;
			if ( optarg != NULL )
				code_filename = strdup ( optarg ) ;
			break ;
		case 'd' :
		case 's' :
			bSCR = true ;
			if ( optarg != NULL )
				data_filename = strdup ( optarg ) ;
			break ;
		case 'h' :
			bOptErr = true ;
			break ;
		case 'v' :
			bVerbose = true ;
			printf ( "! \'verbose\' option chosen\n" ) ;
			break ;
		default :
			fprintf ( stderr, "? unknown option: -%c\n", optopt ) ;
			bOptErr = true ;
			break ;
		}
	}

    if ( bOptErr || code_filename == NULL ) {
		usage ( basename ( argv[ 0 ] ) ) ;
		exit ( -1 ) ;
	}

	if ( code_filename != NULL ) {
		char * p = code_filename ;
		code_filename = duplicate_filename ( code_filename, ".mem" ) ;
		free ( p ) ;
		if ( bVerbose )
            printf ( "! %s file: '%s'\n", "MEM", code_filename ) ;
	}

	if ( data_filename != NULL ) {
		char * p = data_filename ;
		data_filename = duplicate_filename ( data_filename, ".scr" ) ;
		free ( p ) ;
		if ( bVerbose )
            printf ( "! %s file: '%s'\n", "SCR", data_filename ) ;
	}

	// output filename
	if ( argv[ optind ] == NULL )
		vhd_filename = filename ( code_filename ) ;
	else
		vhd_filename = strdup ( argv[ optind++ ] ) ;
	if ( vhd_filename != NULL ) {
		char * p = vhd_filename ;
		vhd_filename = duplicate_filename ( vhd_filename, ".vhd" ) ;
		free ( p ) ;
		if ( bVerbose ) {
			printf ( "! output file: %s\n", vhd_filename ) ;
		}
	}

	for ( i = 0 ; i < MAXMEM ; i++ ) {
		Code[ i ].addr = i ;
		Code[ i ].code = 0 ;
		Code[ i ].blank = true ;
		Code[ i ].breadcrum = false ;
		Code[ i ].label = false ;
	}

	for ( i = 0 ; i < MAXSCR ; i++ )
		Data[ i ] = 0 ;

	if ( bMEM )
		result = loadMEM ( code_filename ) ;
	if ( bSCR )
		result &= loadSCR ( data_filename, 0 ) ;
	if ( ! result )
		exit ( -2 ) ;

	checkPB6 ( 0 ) ;

	result = writeVHD6 ( vhd_filename, Code, Data, inst_map, code_size, stack_size, pad_size, bank_size, want_alu ) ;
	if ( ! result )
		exit ( -2 ) ;

	if ( code_filename )
		free( code_filename ) ;
	if ( data_filename )
		free( data_filename ) ;
	if ( vhd_filename )
		free( vhd_filename ) ;
	exit ( 0 ) ;
}
