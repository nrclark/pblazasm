/*
 *  Copyright © 2003..2010 : Henk van Kampen <henk@mediatronix.com>
 *
 *	This file is part of pBlazDIS.
 *
 *  pBlazMRG is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pBlazDIS is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pBlazDIS.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#include "pbTypes.h"
#include "pbLibgen.h"

#ifdef TCC
#include "getopt.h"
#endif

uint32_t Code[ 1024 ] ;

static void usage( char * text ) {
	printf( "\n%s - %s\n", text, "Picoblaze Disassembler utility V1.0" ) ;
	printf( "\nUSAGE:\n" ) ;
	printf( "   pBlazDIS [-v] <MEM inputfile> <PSM outputfile>\n" ) ;
}

bool loadMEM( const char * strMEMfile ) {
	int i, j, addr ;
	uint32_t code ;
	char line[ 256 ], *p ;
	FILE * infile = NULL ;

	infile = fopen( strMEMfile, "r" ) ;
	if ( infile == NULL ) {
		fprintf( stderr, "? Unable to open MEM file '%s'", strMEMfile ) ;
		return false ;
	}

	for ( i = 0 ; i < 1024 ; i++ )
		Code[ i ] = 0 ;

	for ( addr = -1 ; addr < 1024 + 128 && fgets( line, sizeof( line ), infile ) != NULL; ) {
		if ( ( p = strchr( line, '@' ) ) != NULL ) {
			if ( sscanf( ++p, "%X", &addr ) != 1 ) {
				fprintf( stderr, "? Missing address in MEM file '%s'", strMEMfile ) ;
				return false ;
			}
		} else {
			if ( addr == -1 ) {
				fprintf( stderr, "? Missing address in MEM file '%s'", strMEMfile ) ;
				return false ;
			}
			sscanf( line, "%X", &code ) ;
			Code[ addr ] = code ;
			addr += 1 ;
		}
	}

	fclose( infile ) ;

	return true ;
}

static uint32_t DestReg( const int code ) {
	return ( code >> 8 ) & 0xF ;
}

static uint32_t SrcReg( const int code ) {
	return ( code >> 4 ) & 0xF ;
}

static uint32_t Constant( const int code ) {
	return code & 0xFF ;
}

static uint32_t Address( const int code ) {
	return code & 0x3FF ;
}

static char * Condition( const int code ) {
	const char * Conditions[ 4 ] = { "Z", "NZ", "C", "NC" } ;
	return  Conditions[ ( code >> 10 ) & 0x3 ] ;
}

static bool writePSM( const char * strPSMfile ) {
	FILE * outfile = NULL ;
	int pc = 0 ;
	uint32_t c = 0 ;
	enum {
		stIDLE, stCODE, stDATA
	} state = stIDLE ;

	outfile = fopen( strPSMfile, "w" ) ;
	if ( outfile == NULL ) {
		fprintf( stderr, "? Unable to open output file '%s'", strPSMfile ) ;
		return false ;
	}
	for ( pc = 0 ; pc < 1024 ; ) {
		c = Code[ pc ] & 0x3FFFF ;
		switch ( state ) {
		case stIDLE :
			if ( c != 0 ) {
				switch ( pc ) {
				case 0x380 :
				    fprintf( outfile, "\n\t.SCR\t0x%.3X\n", pc ) ;
					state = stDATA ;
					break ;
				default :
				    fprintf( outfile, "\n\t.ORG\t0x%.3X\n", pc ) ;
					state = stCODE ;
				}
			} else
				pc += 1 ;
			break ;
		case stCODE :
			if ( c != 0 ) {
				switch ( c ) {
				case 0x00000 ... 0x00FFF :
					fprintf( outfile, "\tMOVE\ts%X, 0x%.2X\t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
					break ;
				case 0x00000 ... 0x01FFF :
					fprintf( outfile, "\tMOVE\ts%X, s%X\t; 0x%.5X\n", DestReg( c ), SrcReg( c ), c ) ;
					break ;

				case 0x0A000 ... 0x0AFFF :
					fprintf( outfile, "\tAND \ts%X, 0x%.2X\t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
					break ;
				case 0x0B000 ... 0x0BFFF :
					fprintf( outfile, "\tAND \ts%X, s%X\t; 0x%.5X\n", DestReg( c ), SrcReg( c ), c ) ;
					break ;

				case 0x0C000 ... 0x0CFFF :
					fprintf( outfile, "\tOR  \ts%X, 0x%.2X\t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
					break ;
				case 0x0D000 ... 0x0DFFF :
					fprintf( outfile, "\tOR  \ts%X, s%X\t; 0x%.5X\n", DestReg( c ), SrcReg( c ), c ) ;
					break ;

				case 0x0E000 ... 0x0EFFF :
					fprintf( outfile, "\tXOR \ts%X, 0x%.2X\t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
					break ;
				case 0x0F000 ... 0x0FFFF :
					fprintf( outfile, "\tXOR \ts%X, s%X\t; 0x%.5X\n", DestReg( c ), SrcReg( c ), c ) ;
					break ;

				case 0x12000 ... 0x12FFF :
					fprintf( outfile, "\tTEST\ts%X, 0x%.2X\t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
					break ;
				case 0x13000 ... 0x13FFF :
					fprintf( outfile, "\tTEST\ts%X, s%X\t; 0x%.5X\n", DestReg( c ), SrcReg( c ), c ) ;
					break ;

				case 0x18000 ... 0x18FFF :
					fprintf( outfile, "\tADD \ts%X, 0x%.2X\t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
					break ;
				case 0x19000 ... 0x19FFF :
					fprintf( outfile, "\tADD \ts%X, s%X\t; 0x%.5X\n", DestReg( c ), SrcReg( c ), c ) ;
					break ;

				case 0x1A000 ... 0x1AFFF :
					fprintf( outfile, "\tADDC\ts%X, 0x%.2X\t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
					break ;
				case 0x1B000 ... 0x1BFFF :
					fprintf( outfile, "\tADDC\ts%X, s%X\t; 0x%.5X\n", DestReg( c ), SrcReg( c ), c ) ;
					break ;

				case 0x1C000 ... 0x1CFFF :
					fprintf( outfile, "\tSUB \ts%X, 0x%.2X\t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
					break ;
				case 0x1D000 ... 0x1DFFF :
					fprintf( outfile, "\tSUB \ts%X, s%X\t; 0x%.5X\n", DestReg( c ), SrcReg( c ), c ) ;
					break ;

				case 0x1E000 ... 0x1EFFF :
					fprintf( outfile, "\tSUBC\ts%X, 0x%.2X\t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
					break ;
				case 0x1F000 ... 0x1FFFF :
					fprintf( outfile, "\tSUBC\ts%X, s%X\t; 0x%.5X\n", DestReg( c ), SrcReg( c ), c ) ;
					break ;

				case 0x14000 ... 0x14FFF :
					fprintf( outfile, "\tCOMP\ts%X, 0x%.2X\t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
					break ;
				case 0x15000 ... 0x15FFF :
					fprintf( outfile, "\tCOMP\ts%X, s%X\t; 0x%.5X\n", DestReg( c ), SrcReg( c ), c ) ;
					break ;

				case 0x20000 ... 0x20FFF :
					switch ( c & 0xF ) {
					case 0x2 :
						fprintf( outfile, "\tRL  \ts%X   \t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
						break ;
					case 0x6 :
						fprintf( outfile, "\tSL0 \ts%X   \t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
						break ;
					case 0x7 :
						fprintf( outfile, "\tSL1 \ts%X   \t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
						break ;
					case 0x0 :
						fprintf( outfile, "\tSLA \ts%X   \t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
						break ;
					case 0x4 :
						fprintf( outfile, "\tSLX \ts%X   \t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
						break ;

					case 0xC :
						fprintf( outfile, "\tRR  \ts%X   \t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
						break ;
					case 0xE :
						fprintf( outfile, "\tSR0 \ts%X   \t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
						break ;
					case 0xF :
						fprintf( outfile, "\tSR1 \ts%X   \t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
						break ;
					case 0x8 :
						fprintf( outfile, "\tSRA \ts%X   \t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
						break ;
					case 0xA :
						fprintf( outfile, "\tSRX \ts%X   \t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
						break ;

					default :
						fprintf( outfile, "\tINST\t0x%.5X\t; 0x%.5X\n", c, c ) ;
					}
					break ;

				case 0x34000 ... 0x34FFF :
					fprintf( outfile, "\tJUMP\t0x%.3X\t; 0x%.5X\n", Address( c ), c ) ;
					break ;
				case 0x35000 ... 0x35FFF :
					fprintf( outfile, "\tJUMP\t%s, 0x%.3X\t; 0x%.5X\n", Condition( c ), Address( c ), c ) ;
					break ;

				case 0x30000 ... 0x30FFF :
					fprintf( outfile, "\tCALL\t0x%.3X\t; 0x%.5X\n", Address( c ), c ) ;
					break ;
				case 0x31000 ... 0x31FFF :
					fprintf( outfile, "\tCALL\t%s, 0x%.3X\t; 0x%.5X\n", Condition( c ), Address( c ), c ) ;
					break ;

				case 0x2A000 ... 0x2AFFF :
					fprintf( outfile, "\tRET\t \t; 0x%.5X\n", c ) ;
					break ;
				case 0x2B000 ... 0x2BFFF :
					fprintf( outfile, "\tRET\t%s \t; 0x%.5X\n", Condition( c ), c ) ;
					break ;

				case 0x2E000 ... 0x2EFFF :
					fprintf( outfile, "\tST  \ts%X, 0x%.2X\t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
					break ;
				case 0x2F000 ... 0x2FFFF :
					fprintf( outfile, "\tST  \ts%X, s%X\t; 0x%.5X\n", DestReg( c ), SrcReg( c ), c ) ;
					break ;

				case 0x06000 ... 0x06FFF :
					fprintf( outfile, "\tLD  \ts%X, 0x%.2X\t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
					break ;
				case 0x07000 ... 0x07FFF :
					fprintf( outfile, "\tLD  \ts%X, s%X\t; 0x%.5X\n", DestReg( c ), SrcReg( c ), c ) ;
					break ;

				case 0x2C000 ... 0x2CFFF :
					fprintf( outfile, "\tOUT \ts%X, 0x%.2X\t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
					break ;
				case 0x2D000 ... 0x2DFFF :
					fprintf( outfile, "\tOUT \ts%X, s%X\t; 0x%.5X\n", DestReg( c ), SrcReg( c ), c ) ;
					break ;

				case 0x04000 ... 0x04FFF :
					fprintf( outfile, "\tIN  \ts%X, 0x%.2X\t; 0x%.5X\n", DestReg( c ), Constant( c ), c ) ;
					break ;
				case 0x05000 ... 0x05FFF :
					fprintf( outfile, "\tIN  \ts%X, s%X\t; 0x%.5X\n", DestReg( c ), SrcReg( c ), c ) ;
					break ;

				case 0x3C000 :
					fprintf( outfile, "\tDINT\t \t; 0x%.5X\n", c ) ;
					break ;
				case 0x3C001 :
					fprintf( outfile, "\tEINT\t \t; 0x%.5X\n", c ) ;
					break ;
				case 0x38000 :
					fprintf( outfile, "\tRETI\tDISABLE\t; 0x%.5X\n", c ) ;
					break ;
				case 0x38001 :
					fprintf( outfile, "\tRETI\tENABLE\t; 0x%.5X\n", c ) ;
					break ;

				default :
					fprintf( outfile, "\tINST\t0x%.5X\t; 0x%.5X\n", c, c ) ;
				}
				pc += 1 ;
			} else
				state = stIDLE ;
			break ;
		case stDATA :
			if ( c != 0 ) {
				fprintf( outfile, "\t.BYT\t0x%.2X, 0x%.2X\t; 0x%.5X\n", c & 0xFF, ( c >> 8 ) & 0xFF, c ) ;
				pc += 1 ;
			} else
				state = stIDLE ;
			break ;
		}
	}
}

int main( int argc, char *argv[] ) {
	char mem_filename[ 256 ] = { '\0' } ;
	char psm_filename[ 256 ] = { '\0' } ;

	bool bOptErr = false ;
	bool bVerbose = false ;

	extern char * optarg ;
	extern int optind, optopt, opterr ;
	int optch ;

	opterr = -1 ;
	while ( ( optch = getopt( argc, argv, "hv" ) ) != -1 ) {
		switch ( optch ) {
		case 'h' :
			bOptErr = true ;
			break ;
		case 'v' :
			bVerbose = true ;
			break ;
		default :
			fprintf( stderr, "? unknown option: -%c\n", optopt ) ;
			bOptErr = true ;
			break ;
		}
	}

	if ( bOptErr ) {
		usage( basename( argv[ 0 ] ) ) ;
		exit( -1 ) ;
	}

	// source filename
	if ( argv[ optind ] == NULL ) {
		fprintf( stderr, "? source file missing\n" ) ;
		usage( basename( argv[ 0 ] ) ) ;
		exit( -1 ) ;
	}
	strcpy( mem_filename, argv[ optind++ ] ) ;
	if ( strrchr( mem_filename, '.' ) == NULL )
		strcat( mem_filename, ".mem" ) ;
	if ( bVerbose )
		printf( "! MEM file: %s\n", mem_filename ) ;

	// output filename
	if ( argv[ optind ] == NULL ) {
		strcpy( psm_filename, filename( mem_filename ) ) ;
	} else {
		strcpy( psm_filename, argv[ optind++ ] ) ;
	}
	if ( strrchr( psm_filename, '.' ) == NULL )
		strcat( psm_filename, ".psm" ) ;
	if ( bVerbose )
		printf( "! output file: %s\n", psm_filename ) ;

	if ( loadMEM( mem_filename ) ) {
		writePSM( psm_filename ) ;
		exit( 0 ) ;
	} else
		exit( -2 ) ;
}
