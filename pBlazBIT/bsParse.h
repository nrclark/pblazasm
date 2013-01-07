
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

#ifndef BSPARSE_H_INCLUDED
#define BSPARSE_H_INCLUDED

typedef enum _BitStreamType {
    bstSpartan3, bstSpartan3a, bstSpartan3e, bstSpartan6
} BitStreamType_e ;

typedef enum _FPGAType {
    tyXC3S50 = 0,
    tyXC3S200,
    tyXC3S400,
    tyXC3S1000,
    tyXC3S1500,
    tyXC3S2000,
    tyXC3S4000,
    tyXC3S5000,

    tyXC3S100E,
    tyXC3S250E,
    tyXC3S500E,
    tyXC3S1200E,
    tyXC3S1600E,

    tyXC3S50A,
    tyXC3S200A,
    tyXC3S400A,
    tyXC3S700A,
    tyXC3S1400A,

    tyXC3S50AN,
    tyXC3S200AN,
    tyXC3S400AN,
    tyXC3S700AN,
    tyXC3S1400AN,
    tyXC3SD1800A,
    tyXC3SD3400A,

    // Spartan-6
    tyXC6SLX4,
    tyXC6SLX9,
    tyXC6SLX16,
    tyXC6SLX25,
    tyXC6SLX25T,
    tyXC6SLX45,
    tyXC6SLX45T,
    tyXC6SLX75,
    tyXC6SLX75T,
    tyXC6SLX100,
    tyXC6SLX100T,
    tyXC6SLX150,
    tyXC6SLX150T,

    // Virtex-4L
    tyXC4VLX15,
    tyXC4VLX25,
    tyXC4VLX40,
    tyXC4VLX60,
    tyXC4VLX80,
    tyXC4VLX100,
    tyXC4VLX160,

    // Virtex-4S
    tyXC4VSX25,
    tyXC4VSX35,
    tyXC4VSX55,

    // Virtex-4F
    tyXC4VFX12,
    tyXC4VFX20,
    tyXC4VFX40,
    tyXC4VFX60,
    tyXC4VFX100,
    tyXC4VFX140,
    tyXC4VLX200,

    // Virtex-5L
    tyXC5VLX30,
    tyXC5VLX50,
    tyXC5VLX85,
    tyXC5VLX110,
    tyXC5VLX155,
    tyXC5VLX220,
    tyXC5VLX330,
    tyXC5VLX20T,
    tyXC5VLX30T,
    tyXC5VLX50T,
    tyXC5VLX85T,
    tyXC5VLX110T,
    tyXC5VLX155T,
    tyXC5VLX220T,
    tyXC5VLX330T,

    // Virtex-5S
    tyXC5VSX35T,
    tyXC5VSX50T,
    tyXC5VSX95T,
    tyXC5VSX240T,

    // Virtex-5F
    tyXC5VFX30T,
    tyXC5VFX70T,
    tyXC5VFX100T,
    tyXC5VFX130T,
    tyXC5VFX200T,
    tyXC5VTX150T,
    tyXC5VTX240T,

    // Virtex-6
    tyXC6VHX250T,
    tyXC6VHX255T,
    tyXC6VHX380T,
    tyXC6VHX565T,

    // Virtex-6
    tyXC6VLX75T,
    tyXC6VLX130T,
    tyXC6VLX195T,
    tyXC6VLX240T,
    tyXC6VLX365T,
    tyXC6VLX550T,
    tyXC6VLX760,

    // Virtex-6
    tyXC6VSX315T,
    tyXC6VSX475T,
    tyXQ6VLX130T,
    tyXQ6VLX240T,
    tyXQ6VLX550T,
    tyXQ6VSX315T,
    tyXQ6VSX475T,

    tyLast
} FPGAType_e ;

typedef enum _IDCODE {
    idXC3S50      = 0x0140C093,
    idXC3S200     = 0x01414093,
    idXC3S400     = 0x0141C093,
    idXC3S1000    = 0x01428093,
    idXC3S1500    = 0x01434093,
    idXC3S2000    = 0x01440093,
    idXC3S4000    = 0x01448093,
    idXC3S5000    = 0x01450093,

    idXC3S100E    = 0x01C10093,
    idXC3S250E    = 0x01C1A093,
    idXC3S500E    = 0x01C22093,
    idXC3S1200E   = 0x01C2E093,
    idXC3S1600E   = 0x01C3A093,

    idXC3S50A     = 0x02210093,
    idXC3S200A    = 0x02218093,
    idXC3S400A    = 0x02220093,
    idXC3S700A    = 0x02228093,
    idXC3S1400A   = 0x02230093,

    idXC3S50AN    = 0x02610093,
    idXC3S200AN   = 0x02618093,
    idXC3S400AN   = 0x02620093,
    idXC3S700AN   = 0x02628093,
    idXC3S1400AN  = 0x02630093,
    idXC3SD1800A  = 0x03840093,
    idXC3SD3400A  = 0x0384E093,

    // Spartan-6
    idXC6SLX4     = 0x04000093,
    idXC6SLX9     = 0x04001093,
    idXC6SLX16    = 0x04002093,
    idXC6SLX25    = 0x04004093,
    idXC6SLX25T   = 0x04024093,
    idXC6SLX45    = 0x04008093,
    idXC6SLX45T   = 0x04028093,
    idXC6SLX75    = 0x0400E093,
    idXC6SLX75T   = 0x0402E093,
    idXC6SLX100   = 0x04011093,
    idXC6SLX100T  = 0x04031093,
    idXC6SLX150   = 0x0401D093,
    idXC6SLX150T  = 0x0403D093,

    // Virtex-4L
    idXC4VLX15    = 0x01658093,
    idXC4VLX25    = 0x0167C093,
    idXC4VLX40    = 0x016A4093,
    idXC4VLX60    = 0x016B4093,
    idXC4VLX80    = 0x016D8093,
    idXC4VLX100   = 0x01700093,
    idXC4VLX160   = 0x01718093,

    // Virtex-4S
    idXC4VSX25    = 0x02068093,
    idXC4VSX35    = 0x02088093,
    idXC4VSX55    = 0x020B0093,

    // Virtex-4F
    idXC4VFX12    = 0x01E58093,
    idXC4VFX20    = 0x01E64093,
    idXC4VFX40    = 0x01E8C093,
    idXC4VFX60    = 0x01EB4093,
    idXC4VFX100   = 0x01EE4093,
    idXC4VFX140   = 0x01F14093,
    idXC4VLX200   = 0x01734093,

    // Virtex-5L
    idXC5VLX30    = 0x0286E093,
    idXC5VLX50    = 0x02896093,
    idXC5VLX85    = 0x028AE093,
    idXC5VLX110   = 0x028D6093,
    idXC5VLX155   = 0x028EC093,
    idXC5VLX220   = 0x0290C093,
    idXC5VLX330   = 0x0295C093,
    idXC5VLX20T   = 0x02A56093,
    idXC5VLX30T   = 0x02A6E093,
    idXC5VLX50T   = 0x02A96093,
    idXC5VLX85T   = 0x02AAE093,
    idXC5VLX110T  = 0x02AD6093,
    idXC5VLX155T  = 0x02AEC093,
    idXC5VLX220T  = 0x02B0C093,
    idXC5VLX330T  = 0x02B5C093,

    // Virtex-5S
    idXC5VSX35T   = 0x02E72093,
    idXC5VSX50T   = 0x02E9A093,
    idXC5VSX95T   = 0x02ECE093,
    idXC5VSX240T  = 0x02F3E093,

    // Virtex-5F
    idXC5VFX30T   = 0x03276093,
    idXC5VFX70T   = 0x032C6093,
    idXC5VFX100T  = 0x032D8093,
    idXC5VFX130T  = 0x03300093,
    idXC5VFX200T  = 0x03334093,
    idXC5VTX150T  = 0x04502093,
    idXC5VTX240T  = 0x0453E093,

    // Virtex-6
    idXC6VHX250T  = 0x042A2093,
    idXC6VHX255T  = 0x042A4093,
    idXC6VHX380T  = 0x042A8093,
    idXC6VHX565T  = 0x042AC093,

    // Virtex-6
    idXC6VLX75T   = 0x04244093,
    idXC6VLX130T  = 0x0424A093,
    idXC6VLX195T  = 0x0424C093,
    idXC6VLX240T  = 0x04250093,
    idXC6VLX365T  = 0x04252093,
    idXC6VLX550T  = 0x04256093,
    idXC6VLX760   = 0x0423A093,

    // Virtex-6
    idXC6VSX315T  = 0x04286093,
    idXC6VSX475T  = 0x04288093,
    idXQ6VLX130T  = 0x0424A093,
    idXQ6VLX240T  = 0x04250093,
    idXQ6VLX550T  = 0x04256093,
    idXQ6VSX315T  = 0x04286093,
    idXQ6VSX475T  = 0x04288093
} IDCODE_e ;

// frame lengths
typedef enum _FRLEN {
    // Spartan-3
    flXC3S50 = 37,
    flXC3S200 = 53,
    flXC3S400 = 69,
    flXC3S1000 = 101,
    flXC3S1500 = 133,
    flXC3S2000 = 165,
    flXC3S4000 = 197,
    flXC3S5000 = 213,

    // Spartan-3E
    flXC3S100E = 49,
    flXC3S250E = 73,
    flXC3S500E = 97,
    flXC3S1200E = 125,
    flXC3S1600E = 157,

    // Spartan3A, AN, DA
    flXC3S50A =  74, // all 16bit!
    flXC3S200A  = 138,
    flXC3S400A  = 170,
    flXC3S700A  = 202,
    flXC3S1400A = 298,

    flXC3SD1800A = 362,
    flXC3SD3400A = 426,

    flXC3S50AN =  74, // all 16bit!
    flXC3S200AN  = 138,
    flXC3S400AN  = 170,
    flXC3S700AN  = 202,
    flXC3S1400AN = 298,

    flXC4VLX15 = 41,
    flXC4VLX25 = 41,
    flXC4VLX40 = 41,
    flXC4VLX60 = 41,
    flXC4VLX80 = 41,
    flXC4VLX100 = 41,
    flXC4VLX160 = 41,

    // Virtex-4
    flXC4VSX25 = 41,
    flXC4VSX35 = 41,
    flXC4VSX55 = 41,

    // Virtex-4
    flXC4VFX12 = 41,
    flXC4VFX20 = 41,
    flXC4VFX40 = 41,
    flXC4VFX60 = 41,
    flXC4VFX100 = 41,
    flXC4VFX140 = 41,
    flXC4VLX200 = 41,

    // Virtex-5L
    flXC5VLX30 = 41,
    flXC5VLX50 = 41,
    flXC5VLX85 = 41,
    flXC5VLX110 = 41,
    flXC5VLX155 = 41,
    flXC5VLX220 = 41,
    flXC5VLX330 = 41,
    flXC5VLX20T = 41,
    flXC5VLX30T = 41,
    flXC5VLX50T = 41,
    flXC5VLX85T = 41,
    flXC5VLX110T = 41,
    flXC5VLX155T = 41,
    flXC5VLX220T = 41,
    flXC5VLX330T = 41,

    // Virtex-5S
    flXC5VSX35T = 41,
    flXC5VSX50T = 41,
    flXC5VSX95T = 41,
    flXC5VSX240T = 41,

    // Virtex-5F
    flXC5VFX30T = 41,
    flXC5VFX70T = 41,
    flXC5VFX100T = 41,
    flXC5VFX130T = 41,
    flXC5VFX200T = 41,
    flXC5VTX150T = 41,
    flXC5VTX240T = 41,

    // Virtex-6
    flXC6VHX250T = 81,
    flXC6VHX255T = 81,
    flXC6VHX380T = 81,
    flXC6VHX565T = 81,

    // Virtex-6
    flXC6VLX75T = 81,
    flXC6VLX130T = 81,
    flXC6VLX195T = 81,
    flXC6VLX240T = 81,
    flXC6VLX365T = 81,
    flXC6VLX550T = 81,
    flXC6VLX760 = 81,

    // Virtex-6
    flXC6VSX315T = 81,
    flXC6VSX475T = 81,
    flXQ6VLX130T = 81,
    flXQ6VLX240T = 81,
    flXQ6VLX550T = 81,
    flXQ6VSX315T = 81,
    flXQ6VSX475T = 81
} FRLEN_e ;

typedef struct _INFO {
    int id ; // IDCode
    int fl ; // framelength
    int bs, be ; // blockram start, blockram end
} INFO_t ;


// Table 13: Frame Address Scheme
// Column GCLK_L GCLK_R CENTER TERM_L IOI_L CLB  CLB  CLB  CLB  CLB  CLB  IOI_R TERM_R  BRAM BRAM  BRAM_INIT BRAM_INIT

// Block  0      0      0      0      0     0    0    0    0    0    0    0     0       1    1     2         2
// Major  0      0      0      1      2     3    4    5    6    7    8    9     10      0    1     0         1
// Minor  0      1      2      0~1    0~18  0~18 0~18 0~18 0~18 0~18 0~18 0~18  0~1     0~75 0~75  0~18      0~18

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
    uint32_t autocrc ;
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

bool parse_file ( const char * strBitfile, BitStreamType_e bsType, int bVerbose ) ;
bool merge_code ( uint16_t * code, int len, int nr, int bVerbose ) ;
bool write_file ( const char * strBitfile ) ;
bool get_code ( uint16_t * code, uint32_t * p, int len ) ;

#endif // BSPARSE_H_INCLUDED
