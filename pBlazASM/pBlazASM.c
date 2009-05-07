/*
 *  Copyright © 2003..2008 : Henk van Kampen <henk@mediatronix.com>
 *
 *	This file is part of pBlazASM.
 *
 *  pBlazASM is free software: you can redistribute it and/or modify
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>

#include "pbTypes.h"
#include "pbParser.h"
#include "pbLibgen.h"

/** \brief
 *  pBlazASM main
 */

static void usage(char * text) {
	printf( "\n%s - %s\n", text, "Picoblaze Assembler" ) ;
	printf( "\nUSAGE:\n" ) ;
	printf( "   pBlazASM [-m[MEMfile>]] [-l[LSTfile]] [-k] [-v] <input file>\n" ) ;
}

int main(int argc, char **argv) {
	char src_filename[ 256 ] = { 0 } ;
	char mem_filename[ 256 ] = { 0 } ;
	char list_filename[ 256 ] = { 0 } ;
	char * pfile, *ppath ;

	// KCPSM mode, accepts 'NAMEREG' etc
	bool bKCPSM_mode = false ;
	bool bOptErr = false ;
	bool bWantMEM = false ;
	bool bWantLST = false ;
	bool bVerbose = false ;

	extern char * optarg ;
	extern int optind, optopt ;
	int optch ;

	while ( ( optch = getopt( argc, argv, ":hkl::m::v" ) ) != -1 ) {
		switch ( optch ) {
		case 'h' :
			bOptErr = true ;
			break ;
		case 'm' :
			bWantMEM = true ;
			if ( optarg != NULL )
				strcpy( mem_filename, optarg ) ;
			break ;
		case 'l' :
			bWantLST = true ;
			if ( optarg != NULL )
				strcpy( list_filename, optarg ) ;
			break ;
		case 'k' :
			bKCPSM_mode = true ;
			break ;
		case 'v' :
			bVerbose = true ;
			break ;
		case ':' :
			fprintf( stderr, "? missing option -%c\n", optopt ) ;
			bOptErr = true ;
			break ;
		default :
			fprintf( stderr, "? unknown option -%c\n", optopt ) ;
			bOptErr = true ;
			break ;
		}
	}

	if ( bOptErr ) {
		usage( basename( argv[ 0 ] ) ) ;
		exit( -1 ) ;
	}

	if ( bVerbose && bKCPSM_mode )
		printf( "! KCPSM3 compatible mode selected\n" ) ;

	if ( argv[ optind ] == NULL ) {
		fprintf( stderr, "? source file missing\n" ) ;
		usage( basename( argv[ 0 ] ) ) ;
		exit( -1 ) ;
	}
	strcpy( src_filename, argv[ optind ] ) ;
	if ( strlen( src_filename ) == 0 ) {
		fprintf( stderr, "? source file missing\n" ) ;
		usage( basename( argv[ 0 ] ) ) ;
		exit( -1 ) ;
	}
	if ( strrchr( src_filename, '.' ) == NULL )
		strcat( src_filename, ".psm" ) ;

	if ( bWantMEM ) {
		if ( strlen( mem_filename ) == 0 ) {
			pfile = filename( src_filename ) ;
			ppath = dirname( src_filename ) ;
			strcpy( mem_filename, ppath ) ;
			strcat( mem_filename, "/" ) ;
			strcat( mem_filename, pfile ) ;
			strcat( mem_filename, ".mem" ) ;
			free( ppath ) ;
			free( pfile ) ;
		}
		if ( strrchr( mem_filename, '.' ) == NULL )
			strcat( mem_filename, ".mem" ) ;
		if ( bVerbose )
			printf( "! MEM file: %s\n", mem_filename ) ;
	}

	if ( bWantLST ) {
		if ( strlen( list_filename ) == 0 ) {
			pfile = filename( src_filename ) ;
			ppath = dirname( src_filename ) ;
			strcpy( list_filename, ppath ) ;
			strcat( list_filename, "/" ) ;
			strcat( list_filename, pfile ) ;
			strcat( list_filename, ".lst" ) ;
			free( ppath ) ;
			free( pfile ) ;
		}
		if ( strrchr( list_filename, '.' ) == NULL )
			strcat( list_filename, ".lst" ) ;
		if ( bVerbose )
			printf( "! LST file: %s\n", list_filename ) ;
	}

	if ( assembler( src_filename, mem_filename, list_filename, bKCPSM_mode ) )
		exit( 0 ) ;
	else
		exit( -1 ) ;
}
