
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
#include <time.h>
#include <stdbool.h>

#include "pbTypes.h"
#include "pbLibgen.h"
//#include "pbCRC32.h"

#if defined TCC || defined _MSC_VER
#include "getopt.h"
#else
#include <unistd.h>
#endif

#include "bsParse.h"
#include "version.h"

#define MAXMEM  1024

uint16_t Data[ MAXMEM * 18 / 16 ] ;


static void usage ( char * text ) {
	printf ( "\n%s - Picoblaze Assembler bitstream update utility V%ld.%ld.%ld (%s) (c) 2012-2013 Henk van Kampen\n", text, MAJOR, MINOR, BUILDS_COUNT, STATUS ) ;

    printf ( "\nThis program comes with ABSOLUTELY NO WARRANTY.\n"  ) ;
    printf ( "This is free software, and you are welcome to redistribute it\n"  ) ;
    printf ( "under certain conditions. See <http://www.gnu.org/licenses/>\n"  ) ;

    printf ( "\nUSAGE:\n" ) ;
    printf ( "   pBlazBIT -6 [-v] -b<nr_blockram> -c<MEM code inputfile> [-s<MEM data inputfile] -o<BIT outputfile> <BIT inputfile>\n"
//    printf ( "   pBlazBIT -3|-3a|-3e|-6 [-v] -b<nr_blockram> -c<MEM code inputfile> [-s<MEM data inputfile] -o<BIT outputfile> <BIT inputfile>\n" ) ;
             "   where:\n"
             "         -6      select Spartan-6 bitfile format (only supported format)\n"
             "         -v      generates verbose reporting\n"
             "         -c      loads one or more code MEM files\n"
             "         -s      loads one or more data MEM files\n"
             "         -o      name of the .bit outputfile\n" ) ;
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
    int bVerbose = 0 ;
    BitStreamType_e bsType = bstSpartan6 ;

    extern char * optarg ;
    extern int optind, optopt ;
    int optch, nr = -1 ;

    opterr = -1 ;
    while ( ( optch = getopt ( argc, argv, "b:c:ho:s:v3:6" ) ) != -1 ) {
        switch ( optch ) {
        case 'b' :
            if ( optarg != NULL )
                nr = atoi ( optarg ) ;
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
            bVerbose += 1 ;
            break ;
        case '3' :
            bsType = bstSpartan3 ;
            if ( optarg != NULL )
                switch ( optarg[ 0 ] ) {
                case 'a' :
                    bsType = bstSpartan3a ;
                    if ( bVerbose > 0 )
                        printf ( "! Spartan-3a selected\n" ) ;
                    break ;
                case 'e' :
                    bsType = bstSpartan3e ;
                    if ( bVerbose > 0 )
                        printf ( "! Spartan-3e selected\n" ) ;
                    break ;
                default :
                    fprintf ( stderr, "? unknown FPGA family chosen\n" ) ;
                    usage ( basename ( argv[ 0 ] ) ) ;
                    exit ( -1 ) ;
                }
            else if ( bVerbose )
                printf ( "! Spartan-3 selected\n" ) ;
            break ;
        case '6' :
            bsType = bstSpartan6 ;
            if ( bVerbose > 0 )
                printf ( "! Spartan-6 selected\n" ) ;
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
        if ( bVerbose > 0 )
            printf ( "! source bitstream file: %s\n", source_filename ) ;
    }

    if ( bVerbose > 1 ) {
        if ( ! parse_file ( source_filename, bsType, bVerbose ) )
            exit ( -3 ) ;
        exit( 0 ) ;
    }

    if ( strlen ( dest_filename ) > 0 ) {
        if ( strrchr ( dest_filename, '.' ) == NULL )
            strcat ( dest_filename, ".bit" ) ;
        if ( bVerbose > 0 )
            printf ( "! destination bitstream file: %s\n", dest_filename ) ;
    }

    if ( strlen ( code_filename ) > 0 ) {
        if ( strrchr ( code_filename, '.' ) == NULL )
            strcat ( code_filename, ".mem" ) ;
        if ( bVerbose > 0 )
            printf ( "! code MEM file: %s\n", code_filename ) ;
    }

    if ( strlen ( data_filename ) > 0 ) {
        if ( strrchr ( data_filename, '.' ) == NULL )
            strcat ( data_filename, ".scr" ) ;
        if ( bVerbose > 0 )
            printf ( "! data MEM file: %s\n", data_filename ) ;
    }


    if ( !loadMEM ( code_filename, data_filename ) )
        exit ( -2 ) ;

    if ( ! parse_file ( source_filename, bsType, bVerbose ) )
        exit ( -3 ) ;
    if ( ! merge_code ( Data, MAXMEM, nr, bVerbose ) )
        exit ( -4 ) ;
    if ( ! write_file ( dest_filename ) )
        exit ( -5 ) ;
    exit ( 0 ) ;
}
