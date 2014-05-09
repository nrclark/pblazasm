
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
#include "pbLibgen.h"
#include "pbCRC32.h"

#ifdef TCC
#include "getopt.h"
#endif

#include "bsParse.h"

#define MAXMEM  1024

uint16_t Data[ MAXMEM * 18 / 16 ] ;


static void usage ( const char * text ) {
    fprintf( stdout,"\n%s - %s\n", text, "Picoblaze Assembler bitstream dump utility V0.0" ) ;
    fprintf( stdout,"\nUSAGE:\n" ) ;
    fprintf( stdout,"   pBlazDMP [-3|-6] [-v] [-d<data_stream>]* -o<MEM outputfile> <BIT inputfile>\\n" ) ;
}

bool save_MEM ( const char * strCodefile ) {
    uint32_t Code[ MAXMEM ] ;
    int i, j, addr ;
    FILE * outfile = NULL ;

    // rebuilt array[MAXMEM * 18 / 16] with 16 bit value to array[MAXMEM] with 18 bit values
    for ( i = 0, j = 0 ; i < MAXMEM ; i += 8, j += 9 ) {
        Code[ i + 0 ] = ( ( Data[ j + 0 ] <<  2 ) | ( Data[ j + 1 ] >> 14 ) ) & 0x3FFFF ;
        Code[ i + 1 ] = ( ( Data[ j + 1 ] <<  4 ) | ( Data[ j + 2 ] >> 12 ) ) & 0x3FFFF ;
        Code[ i + 2 ] = ( ( Data[ j + 2 ] <<  6 ) | ( Data[ j + 3 ] >> 10 ) ) & 0x3FFFF ;
        Code[ i + 3 ] = ( ( Data[ j + 3 ] <<  8 ) | ( Data[ j + 4 ] >>  8 ) ) & 0x3FFFF ;
        Code[ i + 4 ] = ( ( Data[ j + 4 ] << 10 ) | ( Data[ j + 5 ] >>  6 ) ) & 0x3FFFF ;
        Code[ i + 5 ] = ( ( Data[ j + 5 ] << 12 ) | ( Data[ j + 6 ] >>  4 ) ) & 0x3FFFF ;
        Code[ i + 6 ] = ( ( Data[ j + 6 ] << 14 ) | ( Data[ j + 7 ] >>  2 ) ) & 0x3FFFF ;
        Code[ i + 7 ] = ( ( Data[ j + 7 ] << 16 ) | ( Data[ j + 8 ] >>  0 ) ) & 0x3FFFF ;
    }

    if ( strlen ( strCodefile ) > 0 ) {
        outfile = fopen ( strCodefile, "w" ) ;
        if ( outfile == NULL ) {
            fprintf( stderr, "? Unable to open MEM file '%s'", strCodefile ) ;
            return false ;
        }

        fprintf( outfile, "@%08X\n", 0 ) ;
        for ( addr = 0 ; addr < MAXMEM ; addr += 1 )
            fprintf( outfile, "%08X\n", Code[ addr ] ) ;

        fclose ( outfile ) ;
    }

    return true ;
}

int main ( int argc, char * argv[] ) {
    char bit_filename[ 256 ] = { '\0' } ;
    char code_filename[ 256 ] = { '\0' } ;

    bool bOptErr = false ;
    bool bVerbose = false ;
    bool bSpartan6 = true ;

    extern char * optarg ;
    extern int optind, optopt ;
    int optch, len ;
    uint32_t data[ 16 ] ;

    opterr = -1 ;
    len = 0 ;
    while ( ( optch = getopt ( argc, argv, "d:ho:v36" ) ) != -1 ) {
        switch ( optch ) {
        case 'd' :
            if ( optarg != NULL )
                sscanf ( optarg, "%x", data + len++ ) ;
            else
                bOptErr = true ;
            break ;
        case 'o' :
            if ( optarg != NULL )
                strcpy ( code_filename, optarg ) ;
            else
                bOptErr = true ;
            break ;
        case 'h' :
            bOptErr = true ;
            break ;
        case 'v' :
            bVerbose = true ;
            break ;
        case '3' :
            bSpartan6 = false ;
            break ;
        case '6' :
            bSpartan6 = true ;
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
        usage ( basename ( argv[ 0 ] ) ) ;
        exit ( -1 ) ;
    }

    // bitstream filename
    if ( argv[ optind ] == NULL ) {
        fprintf( stderr, "? bitstream file missing\n" ) ;
        usage ( basename ( argv[ 0 ] ) ) ;
        exit ( -1 ) ;
    }
    strcpy ( bit_filename, argv[ optind++ ] ) ;

    if ( strlen ( bit_filename ) > 0 ) {
        if ( strrchr ( bit_filename, '.' ) == NULL )
            strcat ( bit_filename, ".bit" ) ;
        if ( bVerbose )
            fprintf( stdout, "! bitstream file: %s\n", bit_filename ) ;
    }

    if ( strlen ( code_filename ) > 0 ) {
        if ( strrchr ( code_filename, '.' ) == NULL )
            strcat ( code_filename, ".mem" ) ;
        if ( bVerbose )
            fprintf( stdout, "! code MEM file: %s\n", code_filename ) ;
    }

    if ( ! parse_file ( bit_filename, bSpartan6, bVerbose ) )
        exit ( -3 ) ;
    if ( ! get_code ( Data, data, len ) )
        exit ( -3 ) ;
    if ( ! save_MEM ( code_filename ) )
        exit ( -4 ) ;
    exit ( 0 ) ;
}
