
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

#include "pbTypes.h"
#include "pbLibgen.h"
#include "pbCRC32.h"

#ifdef TCC
#include "getopt.h"
#endif

#include "bsParse.h"

#define MAXMEM  1024

uint16_t Data[ MAXMEM * 18 / 16 ] ;


static void usage ( char * text ) {
    printf ( "\n%s - %s\n", text, "Picoblaze Assembler bitstream update utility V0.2" ) ;
    printf ( "\nUSAGE:\n" ) ;
    printf ( "   pBlazBIT -3|-6 [-v] -b<nr_blockram> -c<MEM code inputfile> [-s<MEM data inputfile] -o<BIT outputfile> <BIT inputfile>\\n" ) ;
}

bool loadMEM ( const char * strCodefile, const char * strDatafile ) {
    uint32_t Code[ MAXMEM ] ;
    int i, j, addr ;
    uint32_t code ;
    char line[ 256 ], *p ;
    FILE * infile = NULL ;

    for ( i = 0 ; i < MAXMEM; i++ )
        Code[ i ] = 0 ;

    if ( strlen ( strCodefile ) > 0 ) {
        infile = fopen ( strCodefile, "r" ) ;
        if ( infile == NULL ) {
            fprintf ( stderr, "? Unable to open code MEM file '%s'", strCodefile ) ;
            return false ;
        }

        for ( addr = -1 ; addr < MAXMEM && fgets ( line, sizeof ( line ), infile ) != NULL; ) {
            if ( ( p = strchr ( line, '@' ) ) != NULL ) {
                if ( sscanf ( ++p, "%X", &addr ) != 1 ) {
                    fprintf ( stderr, "? Missing address in code MEM file '%s'", strCodefile ) ;
                    return false ;
                }
            } else {
                if ( addr == -1 ) {
                    fprintf ( stderr, "? Missing address in code MEM file '%s'", strCodefile ) ;
                    return false ;
                }
                sscanf ( line, "%X", &code ) ;
                Code[ addr ] = code ;

                addr += 1 ;
            }
        }

        fclose ( infile ) ;
    }

    if ( strlen ( strDatafile ) > 0 ) {
        infile = fopen ( strDatafile, "r" ) ;
        if ( infile == NULL ) {
            fprintf ( stderr, "? Unable to open data MEM file '%s'", strDatafile ) ;
            return false ;
        }

        for ( addr = -1 ; addr < MAXMEM * 2 && fgets ( line, sizeof ( line ), infile ) != NULL; ) {
            if ( ( p = strchr ( line, '@' ) ) != NULL ) {
                if ( sscanf ( ++p, "%X", &addr ) != 1 ) {
                    fprintf ( stderr, "? Missing address in data MEM file '%s'", strDatafile ) ;
                    return false ;
                }
            } else {
                if ( addr == -1 ) {
                    fprintf ( stderr, "? Missing address in data MEM file '%s'", strDatafile ) ;
                    return false ;
                }
                sscanf ( line, "%X", &code ) ;
                if ( ( addr & 1 ) != 0 )
                    Code[ addr / 2 ] |= ( Code[ addr / 2 ] & 0x001FF ) | ( ( code & 0xFF ) << 8 ) ;
                else
                    Code[ addr / 2 ] |= ( Code[ addr / 2 ] & 0x3FF00 ) | ( ( code & 0xFF ) << 0 ) ;
                addr += 1 ;
            }
        }

        fclose ( infile ) ;
    }

    // rebuilt array[MAXMEM] with 18 bit values to array[MAXMEM * 18 / 16] with 16 bit values
    for ( i = 0, j = 0 ; j < MAXMEM ; i += 9, j += 8 ) {
        Data[ i + 0 ] =                             Code[ j + 0 ] >> 2  ;
        Data[ i + 1 ] = ( Code[ j + 0 ] << 14 ) | ( Code[ j + 1 ] >>  4 ) ;
        Data[ i + 2 ] = ( Code[ j + 1 ] << 12 ) | ( Code[ j + 2 ] >>  6 ) ;
        Data[ i + 3 ] = ( Code[ j + 2 ] << 10 ) | ( Code[ j + 3 ] >>  8 ) ;
        Data[ i + 4 ] = ( Code[ j + 3 ] <<  8 ) | ( Code[ j + 4 ] >> 10 ) ;
        Data[ i + 5 ] = ( Code[ j + 4 ] <<  6 ) | ( Code[ j + 5 ] >> 12 ) ;
        Data[ i + 6 ] = ( Code[ j + 5 ] <<  4 ) | ( Code[ j + 6 ] >> 14 ) ;
        Data[ i + 7 ] = ( Code[ j + 6 ] <<  2 ) | ( Code[ j + 7 ] >> 16 ) ;
        Data[ i + 8 ] =   Code[ j + 7 ] ;
    }


    return true ;
}

int main ( int argc, char * argv[] ) {
    char code_filename[ 256 ] = { '\0' } ;
    char data_filename[ 256 ] = { '\0' } ;
    char source_filename[ 256 ] = { '\0' } ;
    char dest_filename[ 256 ] = { '\0' } ;

    bool bOptErr = false ;
    bool bVerbose = false ;
    bool bSpartan6 = true ;

    extern char * optarg ;
    extern int optind, optopt ;
    int optch, nr = -1 ;

    opterr = -1 ;
    while ( ( optch = getopt ( argc, argv, "b:c:ho:s:v36" ) ) != -1 ) {
        switch ( optch ) {
        case 'b' :
            if ( optarg != NULL )
                nr = atoi( optarg ) ;
            else
                bOptErr = true ;
            break ;
        case 's' :
            if ( optarg != NULL )
                strcpy ( data_filename, optarg ) ;
            else
                bOptErr = true ;
            break ;
        case 'o' :
            if ( optarg != NULL )
                strcpy ( dest_filename, optarg ) ;
            else
                bOptErr = true ;
            break ;
        case 'c' :
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
            fprintf ( stderr, "? missing option: -%c\n", optopt ) ;
            bOptErr = true ;
            break ;
        default :
            fprintf ( stderr, "? unknown option: -%c\n", optopt ) ;
            bOptErr = true ;
            break ;
        }
    }

    if ( bOptErr || ( nr < 0 ) ) {
        usage ( basename ( argv[ 0 ] ) ) ;
        exit ( -1 ) ;
    }

    // bitstream filename
    if ( argv[ optind ] == NULL ) {
        fprintf ( stderr, "? bitstream file missing\n" ) ;
        usage ( basename ( argv[ 0 ] ) ) ;
        exit ( -1 ) ;
    }
    strcpy ( source_filename, argv[ optind++ ] ) ;

    if ( strlen ( source_filename ) > 0 ) {
        if ( strrchr ( source_filename, '.' ) == NULL )
            strcat ( source_filename, ".bit" ) ;
        if ( bVerbose )
            printf ( "! source bitstream file: %s\n", source_filename ) ;
    }

    if ( strlen ( dest_filename ) > 0 ) {
        if ( strrchr ( dest_filename, '.' ) == NULL )
            strcat ( dest_filename, ".bit" ) ;
        if ( bVerbose )
            printf ( "! destination bitstream file: %s\n", dest_filename ) ;
    }

    if ( strlen ( code_filename ) > 0 ) {
        if ( strrchr ( code_filename, '.' ) == NULL )
            strcat ( code_filename, ".mem" ) ;
        if ( bVerbose )
            printf ( "! code MEM file: %s\n", code_filename ) ;
    }

    if ( strlen ( data_filename ) > 0 ) {
        if ( strrchr ( data_filename, '.' ) == NULL )
            strcat ( data_filename, ".scr" ) ;
        if ( bVerbose )
            printf ( "! data MEM file: %s\n", data_filename ) ;
    }


    if ( !loadMEM ( code_filename, data_filename ) )
        exit ( -2 ) ;

    if ( !parse_file ( source_filename, bSpartan6, bVerbose ) )
        exit ( -3 ) ;
    if ( ! merge_code ( Data, MAXMEM, nr, bVerbose ) )
        exit ( -4 ) ;
    if ( !write_file ( dest_filename ) )
        exit ( -5 ) ;
    exit ( 0 ) ;
}
