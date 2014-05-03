/*
 *  Copyright © 2003..2010 : Henk van Kampen <henk@mediatronix.com>
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
 *  along with pBlazMRG.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#include "pbLibgen.h"

#if defined TCC || defined _MSC_VER
#include "getopt.h"
#else
#include <unistd.h>
#endif


#include "version.h"

#define MAXMEM	4096
#define MAXFILES 16

uint32_t Code[ MAXMEM ] ;

static void usage( const char * text ) {
	printf ( "\n%s - Picoblaze Assembler merge utility V%ld.%ld.%ld (%s) (c) 2003-2013 Henk van Kampen\n", text, MAJOR, MINOR, BUILDS_COUNT, STATUS ) ;

	printf ( "\nThis program comes with ABSOLUTELY NO WARRANTY.\n"  ) ;
	printf ( "This is free software, and you are welcome to redistribute it\n"  ) ;
	printf ( "under certain conditions. See <http://www.gnu.org/licenses/>\n"  ) ;


	printf( "\nUSAGE:\n" ) ;
	printf( "   pBlazMRG [-v] [-s<MEM data inputfile>[+offset]]* [-c<MEM code inputfile>[+offset]]* -e<entity_name> -t<TPL inputfile> <ROM outputfile>\n"
	        "   where:\n"
	        "         -v      generates verbose reporting\n"
	        "         -s      loads one or more data MEM files\n"
	        "         -c      loads one or more code MEM files\n"
	        "         -e      the entity name in the HDL output file\n"
	        "         -t      HDL template file for the code or data rom\n" ) ;
}

bool loadCode( const char * strCodefile, const int offset ) {
	int addr ;
	uint32_t code ;
	char line[ 256 ], *p ;
	FILE * infile = NULL ;

	infile = fopen( strCodefile, "r" ) ;
	if ( infile == NULL ) {
		fprintf( stderr, "? Unable to open code MEM file '%s'\n", strCodefile ) ;
		return false ;
	}

	for ( addr = -1 ; addr < MAXMEM && fgets( line, sizeof( line ), infile ) != NULL; ) {
		if ( ( p = strchr( line, '@' ) ) != NULL ) {
			if ( sscanf( ++p, "%X", &addr ) != 1 ) {
				fprintf( stderr, "? Bad address in code MEM file '%s'\n", strCodefile ) ;
				return false ;
			}
		} else {
			if ( addr == -1 ) {
				fprintf( stderr, "? Missing address in code MEM file '%s'\n", strCodefile ) ;
				return false ;
			}
			sscanf( line, "%X", &code ) ;
			Code[ addr + offset ] = code ;
			addr += 1 ;
		}
	}

	fclose( infile ) ;
	return true ;
}

bool loadData( const char * strDatafile, const int offset ) {
	int addr ;
	uint32_t code ;
	char line[ 256 ], *p ;
	FILE * infile = NULL ;

	infile = fopen( strDatafile, "r" ) ;
	if ( infile == NULL ) {
		fprintf( stderr, "? Unable to open data MEM file '%s'\n", strDatafile ) ;
		return false ;
	}

	for ( addr = -1 ; addr < MAXMEM * 2 && fgets( line, sizeof( line ), infile ) != NULL; ) {
		if ( ( p = strchr( line, '@' ) ) != NULL ) {
			if ( sscanf( ++p, "%X", &addr ) != 1 ) {
				fprintf( stderr, "? Bad address in data MEM file '%s'\n", strDatafile ) ;
				return false ;
			}
		} else {
			if ( addr == -1 ) {
				fprintf( stderr, "? Missing address in data MEM file '%s'\n", strDatafile ) ;
				return false ;
			}
			sscanf( line, "%X", &code ) ;
			if ( ( addr & 1 ) != 0 )
				Code[ addr / 2 + offset ] |= ( Code[ addr / 2 + offset ] & 0x001FF ) | ( ( code & 0xFF ) << 8 ) ;
			else
				Code[ addr / 2 + offset ] |= ( Code[ addr / 2 + offset ] & 0x3FF00 ) | ( ( code & 0xFF ) << 0 ) ;
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
	uint32_t code, line, bit ;
	int c ;
	int p = 0 ;
	int i ;

	infile = fopen( strTPLfile, "r" ) ;
	if ( infile == NULL ) {
		fprintf( stderr, "? Unable to open template file '%s'\n", strTPLfile ) ;
		return false ;
	}

	outfile = fopen( strROMfile, "w" ) ;
	if ( outfile == NULL ) {
		fprintf( stderr, "? Unable to open output file '%s'\n", strROMfile ) ;
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
				// BYTE based INITs
				if ( strncmp( "[8:0]_INIT_", buffer, 11 ) == 0 ) {
					sscanf( buffer, "[8:0]_INIT_%02X", &line ) ;
					if ( line < 128 )
						for ( i = 31 ; i >= 0; i-- ) {
							fprintf( outfile, "%02X", ( Code[ line * 32 + i ] >> 0 ) & 0xFF ) ;
						}
					state = stCOPY ;
				// parity bits
				} else if ( strncmp( "[8:0]_INITP_", buffer, 12 ) == 0 ) {
					// accumulate all bits 8
					sscanf( buffer, "[8:0]_INITP_%02X", &line ) ;
					if ( line < 16 )
						for ( i = 31 ; i >= 0; i-- ) {
							code  =  ( Code[ ( line * 32 + i ) * 8 + 0 ] >> 8 ) & 0x01 ;
							code |= (( Code[ ( line * 32 + i ) * 8 + 1 ] >> 8 ) & 0x01 ) << 1 ;
							code |= (( Code[ ( line * 32 + i ) * 8 + 2 ] >> 8 ) & 0x01 ) << 2 ;
							code |= (( Code[ ( line * 32 + i ) * 8 + 3 ] >> 8 ) & 0x01 ) << 3 ;
							code |= (( Code[ ( line * 32 + i ) * 8 + 4 ] >> 8 ) & 0x01 ) << 4 ;
							code |= (( Code[ ( line * 32 + i ) * 8 + 5 ] >> 8 ) & 0x01 ) << 5 ;
							code |= (( Code[ ( line * 32 + i ) * 8 + 6 ] >> 8 ) & 0x01 ) << 6 ;
							code |= (( Code[ ( line * 32 + i ) * 8 + 7 ] >> 8 ) & 0x01 ) << 7 ;
							fprintf( outfile, "%02X", code ) ;
						}
					state = stCOPY ;

				} else if ( strncmp( "[17:9]_INIT_", buffer, 12 ) == 0 ) {
					sscanf( buffer, "[17:9]_INIT_%02X", &line ) ;
					if ( line < 128 )
						for ( i = 31 ; i >= 0; i-- ) {
							fprintf( outfile, "%02X", ( Code[ line * 32 + i ] >> 9 ) & 0xFF ) ;
						}
					state = stCOPY ;
				// parity bits
				} else if ( strncmp( "[17:9]_INITP_", buffer, 13 ) == 0 ) {
					// accumulate all bits 17
					sscanf( buffer, "[17:9]_INITP_%02X", &line ) ;
					if ( line < 16 )
						for ( i = 31 ; i >= 0; i-- ) {
							code  =  ( Code[ ( line * 32 + i ) * 8 + 0 ] >> 17 ) & 0x01 ;
							code |= (( Code[ ( line * 32 + i ) * 8 + 1 ] >> 17 ) & 0x01 ) << 1 ;
							code |= (( Code[ ( line * 32 + i ) * 8 + 2 ] >> 17 ) & 0x01 ) << 2 ;
							code |= (( Code[ ( line * 32 + i ) * 8 + 3 ] >> 17 ) & 0x01 ) << 3 ;
							code |= (( Code[ ( line * 32 + i ) * 8 + 4 ] >> 17 ) & 0x01 ) << 4 ;
							code |= (( Code[ ( line * 32 + i ) * 8 + 5 ] >> 17 ) & 0x01 ) << 5 ;
							code |= (( Code[ ( line * 32 + i ) * 8 + 6 ] >> 17 ) & 0x01 ) << 6 ;
							code |= (( Code[ ( line * 32 + i ) * 8 + 7 ] >> 17 ) & 0x01 ) << 7 ;
							fprintf( outfile, "%02X", code ) ;
						}
					state = stCOPY ;

				// WORD based INITs
				} else if ( strncmp( "INIT_", buffer, 5 ) == 0 ) {
					sscanf( buffer, "INIT_%02X", &line ) ;
					if ( line < 128 )
						for ( i = 15 ; i >= 0; i-- ) {
							fprintf( outfile, "%04X", ( Code[ line * 16 + i ] >> 0 ) & 0xFFFF ) ;
						}
					state = stCOPY ;
				// parity bits
				} else if ( strncmp( "INITP_", buffer, 6 ) == 0 ) {
					sscanf( buffer, "INITP_%02X", &line ) ;
					if ( line < 16 )
						for ( i = 31 ; i >= 0; i-- ) {
							code  =  ( Code[ ( line * 32 + i ) * 4 + 0 ] >> 16 ) & 0x03 ;
							code |= (( Code[ ( line * 32 + i ) * 4 + 1 ] >> 16 ) & 0x03 ) << 2 ;
							code |= (( Code[ ( line * 32 + i ) * 4 + 2 ] >> 16 ) & 0x03 ) << 4 ;
							code |= (( Code[ ( line * 32 + i ) * 4 + 3 ] >> 16 ) & 0x03 ) << 6 ;
							fprintf( outfile, "%02X", code ) ;
						}
					state = stCOPY ;

				// bit based INITs
				} else if ( strncmp( "INIT64_", buffer, 6 ) == 0 ) {
					sscanf( buffer, "INIT64_%d", &bit ) ;
					if ( bit < 18 ) {
						for ( i = 15 ; i >= 0 ; i -= 1 ) {
							code  = ( ( ( Code[ i * 4 + 0 ] >> bit ) & 1 ) << 0 ) ;
							code |=	( ( ( Code[ i * 4 + 1 ] >> bit ) & 1 ) << 1 ) ;
							code |=	( ( ( Code[ i * 4 + 2 ] >> bit ) & 1 ) << 2 ) ;
							code |=	( ( ( Code[ i * 4 + 3 ] >> bit ) & 1 ) << 3 ) ;
							fprintf( outfile, "%1X", code ) ;
						}
					}
					state = stCOPY ;
				} else if ( strncmp( "INIT128_", buffer, 6 ) == 0 ) {
					sscanf( buffer, "INIT128_%d", &bit ) ;
					if ( bit < 18 ) {
						for ( i = 31 ; i >= 0 ; i -= 1 ) {
							code  = ( ( ( Code[ i * 4 + 0 ] >> bit ) & 1 ) << 0 ) ;
							code |=	( ( ( Code[ i * 4 + 1 ] >> bit ) & 1 ) << 1 ) ;
							code |=	( ( ( Code[ i * 4 + 2 ] >> bit ) & 1 ) << 2 ) ;
							code |=	( ( ( Code[ i * 4 + 3 ] >> bit ) & 1 ) << 3 ) ;
							fprintf( outfile, "%1X", code ) ;
						}
					}
					state = stCOPY ;
				} else if ( strncmp( "INIT256_", buffer, 8 ) == 0 ) {
					sscanf( buffer, "INIT256_%d", &bit ) ;
					if ( bit < 18 ) {
						for ( i = 63 ; i >= 0 ; i -= 1 ) {
							code  = ( ( ( Code[ i * 4 + 0 ] >> bit ) & 1 ) << 0 ) ;
							code |=	( ( ( Code[ i * 4 + 1 ] >> bit ) & 1 ) << 1 ) ;
							code |=	( ( ( Code[ i * 4 + 2 ] >> bit ) & 1 ) << 2 ) ;
							code |=	( ( ( Code[ i * 4 + 3 ] >> bit ) & 1 ) << 3 ) ;
							fprintf( outfile, "%1X", code ) ;
						}
					}
					state = stCOPY ;

				} else if ( strcmp( "psmname", buffer ) == 0 ) {
					fprintf( outfile, "%s", strEntity ) ;
					state = stCOPY ;
				} else if ( strcmp( "name", buffer ) == 0 ) {
					fprintf( outfile, "%s", strEntity ) ;
					state = stCOPY ;
				} else if ( strcmp( "tool", buffer ) == 0 ) {
					fprintf( outfile, "pBlazMRG" ) ;
					state = stCOPY ;
				} else if ( strcmp( "timestamp", buffer ) == 0 ) {
					time_t rawtime;
					struct tm * timeinfo;
					char date_str[ 256 ] ;

					time( &rawtime ) ;
					timeinfo = localtime( &rawtime ) ;
					strftime( date_str, 256, "%d-%b-%Y %X", timeinfo ) ;
					fprintf( outfile, "%s", date_str ) ;
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

int main( int argc, char * argv[] ) {
	char * code_filenames[ MAXFILES ] ;
	char * data_filenames[ MAXFILES ] ;
	char * tpl_filename = NULL ;
	char * rom_filename = NULL ;
	char * entity_name = NULL ;

	bool bOptErr = false ;
	bool bVerbose = false ;

	int iCodeFileIdx = 0 ;
	int iDataFileIdx = 0 ;
	int iCodeOffsets[ MAXFILES ] = { 0 } ;
	int iDataOffsets[ MAXFILES ] = { 0 } ;

	extern char * optarg ;
	extern int optind, optopt ;
	int err, i, optch ;

	opterr = -1 ;
	err = -1 ;
	while ( ( optch = getopt( argc, argv, ":c:d:e:hm:s:t:v" ) ) != -1 ) {
		switch ( optch ) {
		case 'e' :
			if ( optarg != NULL )
                entity_name	= strdup( optarg ) ;
			else
				bOptErr = true ;
			break ;
		case 'c' :
		case 'm' :
			if ( optarg != NULL ) {
			    if ( iCodeFileIdx >= MAXFILES ) {
                    fprintf( stderr, "? Maximum of code files reached: %d\n", iCodeFileIdx ) ;
			    } else {
			        iCodeOffsets[ iCodeFileIdx ] = 0 ;
                    code_filenames[ iCodeFileIdx ] = strdup( optarg ) ;
                    if ( strrchr( code_filenames[ iCodeFileIdx ], '.' ) == NULL )
                        strcat( code_filenames[ iCodeFileIdx ], ".mem" ) ;
                    if ( bVerbose )
                        printf( "! code MEM file: %s\n", code_filenames[ iCodeFileIdx ] ) ;
                    iCodeFileIdx += 1 ;
			    }
			} else
				bOptErr = true ;
			break ;
		case 'd' :
		case 's' :
			if ( optarg != NULL ) {
			    if ( iDataFileIdx >= MAXFILES ) {
                    fprintf( stderr, "? Maximum of data files reached: %d", iDataFileIdx ) ;
			    } else {
			        iDataOffsets[ iDataFileIdx ] = 0 ;
                    data_filenames[ iDataFileIdx ] = strdup( optarg ) ;
                    if ( strrchr( data_filenames[ iDataFileIdx ], '.' ) == NULL )
                        strcat( data_filenames[ iDataFileIdx ], ".mem" ) ;
                    if ( bVerbose )
                        printf( "! data MEM file: %s\n", data_filenames[ iDataFileIdx ] ) ;
                    iDataFileIdx += 1 ;
			    }
			} else
				bOptErr = true ;
			break ;
		case 't' :
			if ( optarg != NULL ) {
				tpl_filename = strdup( optarg ) ;
                if ( strrchr( tpl_filename, '.' ) == NULL )
                    strcat( tpl_filename, ".vhd" ) ;
                if ( bVerbose )
                    printf( "! template VHD file: %s\n", tpl_filename ) ;
			} else
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
		err = 0 ;
		goto finally ;
	}

	err = -2 ;
	if ( ! tpl_filename ) {
		fprintf( stderr, "? missing template file\n" ) ;
		usage( basename( argv[ 0 ] ) ) ;
		err = 0 ;
		goto finally ;
	}

	if ( ! entity_name && iCodeFileIdx > 0  )
		entity_name = strdup( filename( code_filenames[ 0 ] ) ) ;
	if ( bVerbose )
		printf( "! entity name: %s\n", entity_name ) ;

	// output filename
	if ( argv[ optind ] == NULL && iCodeFileIdx > 0 )
		rom_filename = strdup( filename( code_filenames[ 0 ] ) ) ;
	else
		rom_filename = strdup( argv[ optind++ ] ) ;
	if ( strrchr( rom_filename, '.' ) == NULL )
		strcat( rom_filename, ".vhd" ) ;
	if ( bVerbose )
		printf( "! output file: %s\n", rom_filename ) ;

	// load code and data
	for ( i = 0 ; i < MAXMEM ; i++ )
		Code[ i ] = 0 ;

	err = -3 ;
	for ( i = 0 ; i < iCodeFileIdx ; i += 1 )
		if ( ! loadCode( code_filenames[ i ], iCodeOffsets[ i ] ) )
			goto finally ;
	for ( i = 0 ; i < iDataFileIdx ; i += 1 )
		if ( ! loadData( data_filenames[ i ], iDataOffsets[ i ] ) )
			goto finally ;

	// merge in template
	err = -1 ;
	if ( mergeTPL( tpl_filename, rom_filename, entity_name ) )
		err = 0 ;

finally:
	for ( i = 0 ; i < iCodeFileIdx ; i += 1 )
		if ( code_filenames[ i ] )
			free( code_filenames[ i ] ) ;
	for ( i = 0 ; i < iDataFileIdx ; i += 1 )
		if ( data_filenames[ i ] )
			free( data_filenames[ i ] ) ;
	if ( tpl_filename )
		free( tpl_filename ) ;
	if ( rom_filename )
		free( rom_filename ) ;
	if ( entity_name )
		free( entity_name ) ;
	exit( err ) ;
}
