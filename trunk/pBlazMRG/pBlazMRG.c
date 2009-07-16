/*
 *  Copyright © 2003..2008 : Henk van Kampen <henk@mediatronix.com>
 *
 *	This file is part of pBlazMRG.
 *
 *  pBlazMRG is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pBlazMRG is distributed in the hope that it will be useful,
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
#include <unistd.h>
#include <time.h>

#include "pbTypes.h"
#include "pbLibgen.h"

uint8_t INIT[ 64 ][ 32 ] ;
uint8_t INITP[ 8 ][ 128 ] ;

static void usage( char * text ) {
	printf( "\n%s - %s\n", text, "Picoblaze Assembler merge utility V1.0" ) ;
	printf( "\nUSAGE:\n" ) ;
	printf( "   pBlazMRG [-v] -e<entity_name> <MEM inputfile> <TPL inputfile> <ROM outputfile>\n" ) ;
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

	for ( i = 0 ; i < 64; i++ )
		for ( j = 0 ; j < 32; j++ )
			INIT[ i ][ j ] = 0 ;

	for ( i = 0 ; i < 8; i++ )
		for ( j = 0 ; j < 128; j++ )
			INITP[ i ][ j ] = 0 ;

	for ( addr = -1 ; addr < 1024 && fgets( line, sizeof( line ), infile ) != NULL; ) {
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

			// INIT00 .. INIT3F
			INIT[ addr / 16 ][ ( addr % 16 ) * 2 + 0 ] = code & 0xFF ;
			INIT[ addr / 16 ][ ( addr % 16 ) * 2 + 1 ] = ( code >> 8 ) & 0xFF ;
			// INITP00 .. INITP07
			INITP[ addr / 128 ][ addr % 128 ] = ( code >> 16 ) & 0x3 ;

			addr += 1 ;
		}
	}

	fclose( infile ) ;

	return true ;
}

bool mergeTPL( const char * strTPLfile, const char * strROMfile, const char * strEntity ) {
	FILE * infile = NULL ;
	FILE * outfile = NULL ;
	enum {
		stIDLE, stCOPY, stMERGE
	} state = stIDLE ;
	char buffer[ 65 ] ;
	uint32_t code, line ;
	int c ;
	int p = 0 ;
	int i ;

	infile = fopen( strTPLfile, "r" ) ;
	if ( infile == NULL ) {
		fprintf( stderr, "? Unable to open template file '%s'", strTPLfile ) ;
		return false ;
	}

	outfile = fopen( strROMfile, "w" ) ;
	if ( outfile == NULL ) {
		fprintf( stderr, "? Unable to open output file '%s'", strROMfile ) ;
		fclose( infile ) ;
		return false ;
	}

	while ( ( c = fgetc( infile ) ) != EOF ) {
		switch ( state ) {
		case stIDLE :
			buffer[ 0 ] = '\0' ;
			if ( c == '{' ) {
				state = stMERGE ;
				p = 0 ;
			}
			break ;

		case stCOPY :
			if ( c == '{' ) {
				state = stMERGE ;
				p = 0 ;
			} else {
				fputc( c, outfile ) ;
			}
			break ;

		case stMERGE :
			if ( c != '}' ) {
				if ( p < 64 ) {
					buffer[ p++ ] = c ;
					buffer[ p ] = '\0' ;
				}
			} else if ( strlen( buffer ) > 0 ) {
				if ( strncmp( "INIT_", buffer, 5 ) == 0 ) {
					sscanf( buffer, "INIT_%02X", &line ) ;
					if ( line < 64 ) {
						for ( i = 31 ; i >= 0; i-- ) {
							fprintf( outfile, "%02X", INIT[ line ][ i ] ) ;
						}
					}
					state = stCOPY ;
				} else if ( strncmp( "INITP_", buffer, 6 ) == 0 ) {
					sscanf( buffer, "INITP_%02X", &line ) ;
					if ( line < 8 )
						for ( i = 31 ; i >= 0; i-- ) {
							code = INITP[ line ][ i * 4 + 0 ] | ( INITP[ line ][ i * 4 + 1 ] << 2 )
									| ( INITP[ line ][ i * 4 + 2 ] << 4 ) | ( INITP[ line ][ i * 4 + 3 ] << 6 ) ;
							fprintf( outfile, "%02X", code ) ;
						}
					state = stCOPY ;
				} else if ( strcmp( "name", buffer ) == 0 ) {
					fprintf( outfile, "%s", strEntity ) ;
					state = stCOPY ;
				} else if ( strcmp( "tool", buffer ) == 0 ) {
					fprintf( outfile, "pBlazMRG" ) ;
					state = stCOPY ;
				} else if ( strcmp( "timestamp", buffer ) == 0 ) {
									char date_str[9], time_str[9] ;

									_strdate( date_str ) ;
									_strtime( time_str ) ;
									fprintf( outfile, "%s %s", date_str, time_str ) ;
					state = stCOPY ;
				} else if ( strcmp( "begin template", buffer ) == 0 ) {
					state = stCOPY ;
				} else
					state = stIDLE ;
			} else
				state = stIDLE ;
			break ;
		}
	}

	fclose( outfile ) ;
	fclose( infile ) ;

	return true ;
}

int main( int argc, char *argv[] ) {
	char mem_filename[ 256 ] = { '\0' } ;
	char tpl_filename[ 256 ] = { '\0' } ;
	char rom_filename[ 256 ] = { '\0' } ;
	char entity_name[ 256 ] = { '\0' } ;

	bool bOptErr = false ;
	bool bVerbose = false ;

	extern char * optarg ;
	extern int optind, optopt ;
	int optch ;

	opterr = -1 ;
	while ( ( optch = getopt( argc, argv, ":e:hv" ) ) != -1 ) {
		switch ( optch ) {
		case 'e' :
			if ( optarg != NULL )
				strcpy( entity_name, optarg ) ;
			else
				bOptErr = true ;
			break ;
		case 'h' :
			bOptErr = true ;
			break ;
		case 'v' :
			bVerbose = true ;
			break ;
		case ':' :
			fprintf( stderr, "? missing option: -%c\n", optopt ) ;
			bOptErr = true ;
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

	if ( strlen( entity_name ) == 0 ) {
		strcpy( entity_name, filename( mem_filename ) ) ;
	}
	if ( bVerbose )
		printf( "! entity name: %s\n", entity_name ) ;

	// template filename
	if ( argv[ optind ] == NULL ) {
		strcpy( tpl_filename, "ROM_form_template.vhd" ) ;
	} else {
		strcpy( tpl_filename, argv[ optind++ ] ) ;
	}
	if ( strrchr( tpl_filename, '.' ) == NULL )
		strcat( tpl_filename, ".vhd" ) ;
	if ( bVerbose )
		printf( "! template file: %s\n", tpl_filename ) ;

	// output filename
	if ( argv[ optind ] == NULL ) {
		strcpy( rom_filename, filename( mem_filename ) ) ;
	} else {
		strcpy( rom_filename, argv[ optind++ ] ) ;
	}
	if ( strrchr( rom_filename, '.' ) == NULL )
		strcat( rom_filename, ".vhd" ) ;
	if ( bVerbose )
		printf( "! output file: %s\n", rom_filename ) ;

	if ( loadMEM( mem_filename ) ) {
		mergeTPL( tpl_filename, rom_filename, entity_name ) ;
		exit( 0 ) ;
	} else
		exit( -2 ) ;
}
