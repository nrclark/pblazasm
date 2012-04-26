
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

#include "bsParse.h"

typedef enum _BitStreamType {
    bstSpartan3, bstSpartan6
} BitStreamType_e ;

const char * sRegisterNames[] = {
    "CRC", "FARMAJ", "FARMIN", "FDRI", "FDRO",
    "CMD", "CTL", "MASK", "STAT", "LOUT", "COR1",
    "COR2", "PWRDN_REG", "FLR", "IDCODE", "CWDT", "HC_OPT_REG",
    "CSBO", "GENERAL1", "GENERAL2", "GENERAL3", "GENERAL4",
    "GENERAL5", "MODE_REG", "PU_GWE", "PU_GTS", "MFWR", "CCLK_FREQ",
    "SEU_OPT", "EXP_SIGN", "RDBK_SIGN", "BOOSTS", "EYE_MASK",
    "CBC_REG", "Count"
} ;

const char * sOpcodeNames[] = {
    "NOP", "READ", "WRITE"
} ;

static size_t nLength ;
static void * pRaw, * current ;

static uint8_t InitialHeader[ 9 ] = { 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x00 } ;

typedef struct _SpartanBitfileHeader {
    char * info ;
    char * part ;
    char * date ;
    char * time ;
    bool bBit ;
} SpartanBitfileHeader_t ;

typedef struct _Spartan3Packet {
    uint32_t header ;
    uint32_t count ;
    uint32_t * data ;
} Spartan3Packet_t ;

typedef struct _Spartan6Packet {
    uint32_t header ;
    uint32_t count ;
    uint16_t * data ;
    uint32_t autocrc ;
} Spartan6Packet_t ;

typedef struct _SpartanBitfile {
    BitStreamType_e type ;
    SpartanBitfileHeader_t header ;
    uint32_t header_length ;
    uint32_t length ;
    uint32_t count ;
    union {
        Spartan3Packet_t * packets3 ;
        Spartan6Packet_t * packets6 ;
    } ;
} SpartanBitfile_t ;

SpartanBitfile_t bit_file ;


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
    result = strdup ( current ) ;
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
    while ( *b != 0 )
        put_byte ( *b++ ) ;
    put_byte ( 0 ) ;
}

bool parse_header ( int len ) {
    bool result = true ;
    int i ;

    if ( ( * ( uint32_t * ) current ) != 0xFFFFFFFF ) {
        result &= get_word() == sizeof ( InitialHeader ) ;
        for ( i = 0 ; result && i < sizeof ( InitialHeader ) ; i += 1 )
            result &= InitialHeader[ i ] == get_byte() ;
        if ( ! result )
            return result ;

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
    int i, n ;
    uint32_t header, type, count ;
    uint32_t * data ;

    n = 0 ;
    while ( current < pRaw + bit_file.length + bit_file.header_length ) {
        // get packet header
        header = get_long() ;
        type = header >> 29 ;

        // get packet length
        switch ( type ) {
        case 1:
            count = header & 0x0000001f ;
            break ;
        case 2:
            count = header & 0x0 ;
            break ;
        default:
            count = 0 ;
        }
        data = NULL ;
        if ( count > 0 ) {
            data = malloc ( count * sizeof ( uint32_t ) ) ;
            for ( i = 0 ; i < count ; i += 1 )
                data[ i ] = get_long() ;
        }

        bit_file.packets3 = realloc ( bit_file.packets3, ( n + 1 ) * sizeof ( Spartan6Packet_t ) ) ;
        bit_file.packets3[ n ].header = header ;
        bit_file.packets3[ n ].data = data ;
        bit_file.packets3[ n ].count = count ;
        n += 1 ;
    }
    bit_file.count = n ;
    return result ;
}

bool parse_packets6 ( void ) {
    bool result = true ;
    int i, n ;
    uint16_t header, type ;
    uint32_t count, autocrc ;
    uint16_t * data ;

    n = 0 ;
    while ( current < pRaw + bit_file.length + bit_file.header_length ) {
        // get packet header
        header = get_word() ;
        type = header >> 13 ;

        // get packet length
        switch ( type ) {
        case 1:
            count = header & 0x0000001f ;
            break ;
        case 2:
            count = get_long() & 0x0fffffff ;
            break ;
        default:
            count = 0 ;
        }
        data = NULL ;
        if ( count > 0 ) {
            data = malloc ( count * sizeof ( uint16_t ) ) ;
            for ( i = 0 ; i < count ; i += 1 )
                data[ i ] = get_word() ;
        }
        if ( type == 2 )
            autocrc = get_long() ;

        bit_file.packets6 = realloc ( bit_file.packets6, ( n + 1 ) * sizeof ( Spartan6Packet_t ) ) ;
        bit_file.packets6[ n ].header = header ;
        bit_file.packets6[ n ].data = data ;
        bit_file.packets6[ n ].count = count ;
        bit_file.packets6[ n ].autocrc = autocrc ;
        n += 1 ;
    }
    bit_file.count = n ;
    return result ;
}

void show_file ( void ) {
    int i, j ;

    if ( bit_file.header.bBit ) {
        printf ( "! header:\n" ) ;
        printf ( "! info: %s\n", bit_file.header.info ) ;
        printf ( "! part: %s\n", bit_file.header.part ) ;
        printf ( "! date: %s\n", bit_file.header.date ) ;
        printf ( "! time: %s\n", bit_file.header.time ) ;
    }

    printf ( "! number of packets: %d\n", bit_file.count ) ;
    for ( i = 0 ; i < bit_file.count ; i += 1 ) {
        switch ( bit_file.type ) {
        case bstSpartan6:
            printf ( "! %4d: 0x%04X", i, bit_file.packets6[ i ].header ) ;
            printf ( " %6s " , sOpcodeNames[ ( bit_file.packets6[ i ].header >> 11 ) & 0x0003 ] ) ;
            if ( ( ( bit_file.packets6[ i ].header >> 11 ) & 0x0003 ) != 0 ) {
                printf ( " %10s" , sRegisterNames[ ( bit_file.packets6[ i ].header >> 5 ) & 0x003F ] ) ;
                printf ( "  count:  %d", bit_file.packets6[ i ].count ) ;
                for ( j = 0 ; j < 3 && j < bit_file.packets6[ i ].count ; j += 1 )
                    printf ( " : 0x%04X", bit_file.packets6[ i ].data[ j ] ) ;
                if ( bit_file.packets6[ i ].header >> 13 == 2 )
                    printf ( " ... autocrc: 0x%08X", bit_file.packets6[ i ].autocrc ) ;
            }
            printf ( "\n" ) ;
            break;
        case bstSpartan3:
            printf ( "! %4d: 0x%08X", i, bit_file.packets3[ i ].header ) ;
            printf ( " count:  %d", bit_file.packets3[ i ].count ) ;
            if ( bit_file.packets3[ i ].count > 0 )
                printf ( " : 0x%08X\n", bit_file.packets3[ i ].data[ 0 ] ) ;
            else
                printf ( "\n" ) ;
            break;
        }
    }
}

bool merge_code ( uint16_t * code, int len, int nr ) {
    int i, j, n, s ;
    uint16_t data, * packet, * p ;
    int state ;

    // find bulk frame

    n = 0 ;
    for ( i = 0 ; i < bit_file.count ; i += 1 ) {
        if ( bit_file.packets6[ i ].header == 0x5060 ) {
            state = -1 ;
            for ( j = 0 ; j < bit_file.packets6[ i ].count ; j += 1 ) {
                data = bit_file.packets6[ i ].data[ j ] ;
                if ( state == -1 )
                    s = j ;
                switch ( data ) {
                    /* 0 */
                case 0xf01c :
                    if ( nr == 0 )
                        state = 0 ;
                    break ;
                case 0xcaf4 :
                    if ( nr == 1 )
                        state = 0 ;
                    break ;
                case 0xc90b :
                    if ( nr == 2 )
                        state = 0 ;
                    break ;
                case 0xc4b3 :
                    if ( nr == 3 )
                        state = 0 ;
                    break ;
                case 0xe030 :
                    if ( nr == 4 )
                        state = 0 ;
                    break ;
                case 0xe78d :
                    if ( nr == 5 )
                        state = 0 ;
                    break ;
                case 0xf199 :
                    if ( nr == 6 )
                        state = 0 ;
                    break ;
                case 0xe481 :
                    if ( nr == 7 )
                        state = 0 ;
                    break ;
                case 0xcac2 :
                    if ( nr == 8 )
                        state = 0 ;
                    break ;
                case 0xc991 :
                    if ( nr == 9 )
                        state = 0 ;
                    break ;
                case 0xe1eb :
                    if ( nr == 10 )
                        state = 0 ;
                    break ;
                case 0xd3ec :
                    if ( nr == 11 )
                        state = 0 ;
                    break ;
                case 0xefa9 :
                    if ( nr == 12 )
                        state = 0 ;
                    break ;
                case 0xe83b :
                    if ( nr == 13 )
                        state = 0 ;
                    break ;
                case 0xe5dc :
                    if ( nr == 14 )
                        state = 0 ;
                    break ;
                case 0xd366 :
                    if ( nr == 15 )
                        state = 0 ;
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
                    if ( state == 0 )
                        state = 1 ;
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
                    if ( state == 1 )
                        state = 2 ;
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
                    if ( state == 2 )
                        state = 3 ;
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
                    if ( state == 3 )
                        state = 4 ;
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
                    if ( state == 4 )
                        state = 5 ;
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
                    if ( state == 5 )
                        state = 6 ;
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
                    if ( state == 6 )
                        state = 7 ;
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
                        for ( i = 0, data = 0, p = packet + 9 ; i < len * 18 / 16 - 9 ; i += 1 )
                            data |= *p++ ;
                        if ( data != 0 ) {
                            fprintf ( stderr, "? Could not find correctly pre-initialized blockram nr: '%d' in bitfile\n", nr ) ;
                            return false ;
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

    for ( i = 0 ; i < bit_file.count ; i += 1 ) {
        if ( bit_file.packets6[ i ].header == 0x5060 ) {
            state = 0 ;
            for ( j = 0 ; j < bit_file.packets6[ i ].count ; j += 1 ) {
                if ( state == 0 )
                    s = j ;
                data = bit_file.packets6[ i ].data[ j ] ;
                if ( data == p[ state ] )
                    state += 1 ;
                else if ( state > 0 )
                    state = 0 ;
                if ( state >= len )
                    break ;
            }
            if ( state > 0 ) {
                for ( j = 0 ; j < 1024 * 18 / 16 ; j += 1, s += 1 )
                    code[ j ] = bit_file.packets6[ i ].data[ s ] ;
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
        for ( i = 0 ; i < sizeof ( InitialHeader ) ; i += 1 )
            put_byte ( InitialHeader[ i ] ) ;
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
    for ( i = 0 ; i < bit_file.count ; i += 1 ) {
        put_word ( bit_file.packets6[ i ].header ) ;
        if ( bit_file.packets6[ i ].header >> 13 == 2 )
            put_long ( bit_file.packets6[ i ].count ) ;
        for ( j = 0 ; j < bit_file.packets6[ i ].count ; j += 1 )
            put_word ( bit_file.packets6[ i ].data[ j ] ) ;
        if ( bit_file.packets6[ i ].header >> 13 == 2 )
            put_long ( bit_file.packets6[ i ].autocrc ) ;
    }
}

bool parse_file ( const char * strBitfile, bool bSpartan6, bool bVerbose ) {
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
    pRaw = malloc ( sizeof ( char ) * nSize ) ;
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
    if ( bSpartan6 ) {
        bit_file.packets6 = NULL ;
        bit_file.type = bstSpartan6 ;
    } else {
        bit_file.packets3 = NULL ;
        bit_file.type = bstSpartan3 ;
    }

    // sync words should follow
    while ( result ) {
        sync = get_long() ;
        if ( sync == 0xFFFFFFFF )
            ;
        else if ( sync == 0xAA995566 )
            break ;
        else {
            fprintf ( stderr, "? sync word not found\n" ) ;
            goto _free ;
        }
    }

    // the actual work
    if ( bSpartan6 )
        result = result && parse_packets6() ;
    else
        result = result && parse_packets3() ;

    // report
    if ( result && bVerbose )
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
    pRaw = calloc ( bit_file.length + bit_file.header_length, sizeof ( char ) ) ;

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




