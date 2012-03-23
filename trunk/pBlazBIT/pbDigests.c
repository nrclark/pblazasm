
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

#include "md5.h"

static uint32_t _Digests[] = {
    0x3b4b1, 0x347bc, 0x35228, 0x32873, 0x31f1a, 0x3016b, 0x3fa72, 0x3c073,
    0x396a3, 0x3be3c, 0x3f272, 0x3e017, 0x3046d, 0x31b26, 0x374a5, 0x32bd3,
    0x3a2ef, 0x3406e, 0x32c23, 0x351e0, 0x3b9e8, 0x30029, 0x3c909, 0x3242d,
    0x3e45e, 0x3e7ce, 0x37e88, 0x3149a, 0x3f8dd, 0x332b2, 0x37f95, 0x312ce,

    0x37d06, 0x36543, 0x38e81, 0x3d8ec, 0x3eb98, 0x3c1e3, 0x31fca, 0x380c1,
    0x3751d, 0x331dd, 0x36b56, 0x3b26b, 0x329da, 0x3c2c0, 0x3e183, 0x39e34,
    0x3faea, 0x3c4e1, 0x3eef3, 0x307c2, 0x3ab7b, 0x30a38, 0x321e6, 0x3c667,
    0x3d72d, 0x3187d, 0x3f41e, 0x310ea, 0x37d9f, 0x3cdc7, 0x3f590, 0x39205,

    0x3fad6, 0x3f4e6, 0x314a2, 0x312e8, 0x30c67, 0x3249a, 0x3666d, 0x32b09,
    0x30a80, 0x305f5, 0x3594b, 0x3d670, 0x341f8, 0x38c61, 0x39619, 0x32646,
    0x3e99b, 0x3b337, 0x327d3, 0x33831, 0x34912, 0x3e86f, 0x3bdec, 0x387af,
    0x33470, 0x317fd, 0x31e37, 0x3a22f, 0x3af91, 0x38759, 0x381bc, 0x34fb3,

    0x30fa2, 0x3fc59, 0x354ab, 0x340ec, 0x3fe65, 0x38625, 0x3c4ad, 0x3bea5,
    0x323a3, 0x326d0, 0x3b546, 0x3a12f, 0x33fd2, 0x33a27, 0x38dfb, 0x3a0ed,
    0x3e50c, 0x30cb9, 0x3a8de, 0x3f39e, 0x367ea, 0x32d35, 0x30ab1, 0x39773,
    0x37dff, 0x351ca, 0x38eb9, 0x39012, 0x32513, 0x3f24f, 0x3fdaa, 0x34d9a
} ;

void print_CASEs ( void ) {
    int i, j ;
    uint16_t Data[ 16 * 9 ] ;

    for ( i = 0, j = 0 ; i < 16 * 9 ; i += 9, j += 8 ) {
        Data[ i + 0 ] =                                 _Digests[ j + 7 ] >> 2  ;
        Data[ i + 1 ] = ( _Digests[ j + 7 ] << 14 ) | ( _Digests[ j + 6 ] >>  4 ) ;
        Data[ i + 2 ] = ( _Digests[ j + 6 ] << 12 ) | ( _Digests[ j + 5 ] >>  6 ) ;
        Data[ i + 3 ] = ( _Digests[ j + 5 ] << 10 ) | ( _Digests[ j + 4 ] >>  8 ) ;
        Data[ i + 4 ] = ( _Digests[ j + 4 ] <<  8 ) | ( _Digests[ j + 3 ] >> 10 ) ;
        Data[ i + 5 ] = ( _Digests[ j + 3 ] <<  6 ) | ( _Digests[ j + 2 ] >> 12 ) ;
        Data[ i + 6 ] = ( _Digests[ j + 2 ] <<  4 ) | ( _Digests[ j + 1 ] >> 14 ) ;
        Data[ i + 7 ] = ( _Digests[ j + 1 ] <<  2 ) | ( _Digests[ j + 0 ] >> 16 ) ;
        Data[ i + 8 ] =   _Digests[ j + 0 ] ;
    }
    for ( i = 0 ; i < 9 ; i += 1 ) {
        printf ( "/* %d */\n", i ) ;
        for ( j = 0 ; j < 16 ; j += 1 ) {
            printf ( "\tcase 0x%04x : \n", Data[ j * 9 + i ] ) ;
            if ( i == 0 )
                printf ( "\t\tif ( nr == %d ) \n\t\t\tstate = 0 ; \n\t\tbreak ; \n", j ) ;
            else if ( j == 15 )
                printf ( "\t\tif ( state == %d ) \n\t\t\tstate = %d ;\n\t\tbreak ;\n", i - 1, i ) ;
        }
    }
}

//      X"0000_0000_0000_0000_0000_0000_0000_0000_b4b1_47bc_5228_2873_1f1a_016b_fa72_c073",
//      X"0000_0000_0000_0000_0000_0000_0000_0000_96a3_be3c_f272_e017_046d_1b26_74a5_2bd3",
//      X"0000_0000_0000_0000_0000_0000_0000_0000_a2ef_406e_2c23_51e0_b9e8_0029_c909_242d",
//      X"0000_0000_0000_0000_0000_0000_0000_0000_e45e_e7ce_7e88_149a_f8dd_32b2_7f95_12ce",

//      X"0000_0000_0000_0000_0000_0000_0000_0000_7d06_6543_8e81_d8ec_eb98_c1e3_1fca_80c1",
//      X"0000_0000_0000_0000_0000_0000_0000_0000_751d_31dd_6b56_b26b_29da_c2c0_e183_9e34",
//      X"0000_0000_0000_0000_0000_0000_0000_0000_faea_c4e1_eef3_07c2_ab7b_0a38_21e6_c667",
//      X"0000_0000_0000_0000_0000_0000_0000_0000_d72d_187d_f41e_10ea_7d9f_cdc7_f590_9205",

//      X"0000_0000_0000_0000_0000_0000_0000_0000_fad6_f4e6_14a2_12e8_0c67_249a_666d_2b09",
//      X"0000_0000_0000_0000_0000_0000_0000_0000_0a80_05f5_594b_d670_41f8_8c61_9619_2646",
//      X"0000_0000_0000_0000_0000_0000_0000_0000_e99b_b337_27d3_3831_4912_e86f_bdec_87af",
//      X"0000_0000_0000_0000_0000_0000_0000_0000_3470_17fd_1e37_a22f_af91_8759_81bc_4fb3",

//      X"0000_0000_0000_0000_0000_0000_0000_0000_0fa2_fc59_54ab_40ec_fe65_8625_c4ad_bea5",
//      X"0000_0000_0000_0000_0000_0000_0000_0000_23a3_26d0_b546_a12f_3fd2_3a27_8dfb_a0ed",
//      X"0000_0000_0000_0000_0000_0000_0000_0000_e50c_0cb9_a8de_f39e_67ea_2d35_0ab1_9773",
//      X"0000_0000_0000_0000_0000_0000_0000_0000_7dff_51ca_8eb9_9012_2513_f24f_fdaa_4d9a"

void print_INITs ( void ) {
    int i ;

    for ( i = 0 ; i < 16 * 8 ; i += 8 ) {
        printf ( "X\"0000_0000_0000_0000_0000_0000_0000_0000_%04x_%04x_%04x_%04x_%04x_%04x_%04x_%04x,\n",
                 _Digests[ i + 0 ] & 0xffff, _Digests[ i + 1 ] & 0xffff, _Digests[ i + 2 ] & 0xffff, _Digests[ i + 3 ] & 0xffff,
                 _Digests[ i + 4 ] & 0xffff, _Digests[ i + 5 ] & 0xffff, _Digests[ i + 6 ] & 0xffff, _Digests[ i + 7 ] & 0xffff ) ;
    }
}

void main ( void ) {
//    uint8_t digest[ 16 ] ;

//    MD5Init() ;
//    MD5Update( (uint8_t *)"00", 2 ) ;
//    MD5Final( digest ) ;

    print_INITs() ;
    exit ( 0 ) ;
}
