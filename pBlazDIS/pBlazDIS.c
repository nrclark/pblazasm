/*
 *  Copyright © 2003..2010 : Henk van Kampen <henk@mediatronix.com>
 *
 *  This file is part of pBlazDIS.
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

#define MEMSIZE 4096

uint32_t Code[ MEMSIZE ] ;

static void usage ( char * text ) {
    printf ( "\n%s - %s\n", text, "Picoblaze Disassembler utility V2.1" ) ;
    printf ( "\nUSAGE:\n" ) ;
    printf ( "   pBlazDIS [-6] [-x] [-p] [-v] -c<MEM code inputfile> -s<MEM data inputfile> <PSM/VHD outputfile>\n" ) ;
    printf ( "   where:\n"
             "         -c      loads a code MEM file\n"
             "         -s      loads a data MEM file\n"
             "         -6      instruvts to use Picoblaze6 opcodes\n"
             "         -v      generates verbose reporting\n"
             "         -p      generate a PicoCore VHDL file\n" ) ;
}

bool loadMEM ( const char * strMEMfile ) {
    int addr ;
    uint32_t code ;
    char line[ 256 ], *p ;
    FILE * infile = NULL ;

    if ( strMEMfile == NULL || * strMEMfile == 0 )
        return true ;

    infile = fopen ( strMEMfile, "r" ) ;
    if ( infile == NULL ) {
        fprintf ( stderr, "? Unable to open MEM file '%s'", strMEMfile ) ;
        return false ;
    }

    for ( addr = -1 ; addr < MEMSIZE + 128 && fgets ( line, sizeof ( line ), infile ) != NULL; ) {
        if ( ( p = strchr ( line, '@' ) ) != NULL ) {
            if ( sscanf ( ++p, "%X", &addr ) != 1 ) {
                fprintf ( stderr, "? Error in address in MEM file '%s'", strMEMfile ) ;
                return false ;
            }
        } else {
            if ( addr == -1 ) {
                fprintf ( stderr, "? Missing address in MEM file '%s', assuming 0", strMEMfile ) ;
                addr = 0 ;
                // return false ;
            }
            sscanf ( line, "%X", &code ) ;
            Code[ addr ] = code ;
            addr += 1 ;
        }
    }

    fclose ( infile ) ;
    return true ;
}

bool loadXDL ( const char * strXDLfile ) {
    int i, addr, page ;
    uint32_t code ;
    char line[ 256 ], *p, t[ 16 ] ;
    FILE * infile = NULL ;

    if ( strXDLfile == NULL || * strXDLfile == 0 )
        return true ;

    infile = fopen ( strXDLfile, "r" ) ;
    if ( infile == NULL ) {
        fprintf ( stderr, "? Unable to open XDL file '%s'", strXDLfile ) ;
        return false ;
    }

    for ( i = 0 ; i < MEMSIZE ; i++ )
        Code[ i ] = 0 ;

    for ( addr = -1 ; addr < MEMSIZE + 128 && fgets ( line, sizeof ( line ), infile ) != NULL; ) {
        if ( ( p = strstr ( line, "INITP_" ) ) != NULL ) {
            p += 6 ;
            strncpy ( t, p, 2 ) ;
            t[ 2 ] = 0 ;
            if ( sscanf ( t, "%X", &page ) != 1 ) {
                fprintf ( stderr, "? Error in address in XDL file '%s'", strXDLfile ) ;
                return false ;
            } else
                addr = ( page << 7 ) + 127 ;
            p += 4 ;
            for ( i = 0 ; i < 64 ; i += 1 ) {
                strncpy ( t, p, 1 ) ;
                t[ 1 ] = 0 ;
                if ( sscanf ( t, "%X", &code ) != 1 ) {
                    fprintf ( stderr, "? Error in parity data in XDL file '%s'", strXDLfile ) ;
                    return false ;
                }
                Code[ addr-- ] |= ( code << 14 ) & 0x30000 ;
                Code[ addr-- ] |= ( code << 16 ) & 0x30000 ;
                p += 1 ;
            }
        } else if ( ( p = strstr ( line, "INIT_" ) ) != NULL ) {
            p += 5 ;
            strncpy ( t, p, 2 ) ;
            t[ 2 ] = 0 ;
            if ( sscanf ( t, "%X", &page ) != 1 ) {
                fprintf ( stderr, "? Error in address in XDL file '%s'", strXDLfile ) ;
                return false ;
            } else
                addr = ( page << 4 ) + 15 ;
            p += 4 ;
            for ( i = 0 ; i < 16 ; i += 1 ) {
                strncpy ( t, p, 4 ) ;
                t[ 4 ] = 0 ;
                if ( sscanf ( t, "%X", &code ) != 1 ) {
                    fprintf ( stderr, "? Error in data in XDL file '%s'", strXDLfile ) ;
                    return false ;
                }
                Code[ addr-- ] |= code ;
                p += 4 ;
            }
        }
    }

    fclose ( infile ) ;
    return true ;
}

static uint32_t DestReg ( const int code ) {
    return ( code >> 8 ) & 0xF ;
}

static uint32_t SrcReg ( const int code ) {
    return ( code >> 4 ) & 0xF ;
}

static uint32_t Constant ( const int code ) {
    return code & 0xFF ;
}

static uint32_t Address10 ( const int code ) {
    return code & 0x3FF ;
}

static uint32_t Address12 ( const int code ) {
    return code & 0xFFF ;
}

static const char * Condition ( const int code ) {
    const char * Conditions[ 4 ] = { "Z", "NZ", "C", "NC" } ;
    return  Conditions[ ( code >> 10 ) & 0x3 ] ;
}

static bool writePSM3 ( const char * strPSMfile ) {
    FILE * outfile = NULL ;
    int pc = 0 ;
    uint32_t c = 0 ;
    enum {
        stIDLE, stCODE, stDATA
    } state = stIDLE ;

    outfile = fopen ( strPSMfile, "w" ) ;
    if ( outfile == NULL ) {
        fprintf ( stderr, "? Unable to open output file '%s'", strPSMfile ) ;
        return false ;
    }
    for ( pc = 0 ; pc < 1024 ; ) {
        c = Code[ pc ] & 0x3FFFF ;
        switch ( state ) {
        case stIDLE :
            if ( c != 0 ) {
                switch ( pc ) {
                case 0x380 :
                    fprintf ( outfile, "\n\t.SCR\t0x%03X\n", pc ) ;
                    state = stDATA ;
                    break ;
                default :
                    fprintf ( outfile, "\n\t.ORG\t0x%03X\n", pc ) ;
                    state = stCODE ;
                }
            } else
                pc += 1 ;
            break ;
        case stCODE :
            if ( c != 0 ) {
                switch ( c ) {
                case 0x00000 ... 0x00FFF :
                    fprintf ( outfile, "\tMOVE\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x01000 ... 0x01FFF :
                    fprintf ( outfile, "\tMOVE\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x0A000 ... 0x0AFFF :
                    fprintf ( outfile, "\tAND \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x0B000 ... 0x0BFFF :
                    fprintf ( outfile, "\tAND \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x0C000 ... 0x0CFFF :
                    fprintf ( outfile, "\tOR  \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x0D000 ... 0x0DFFF :
                    fprintf ( outfile, "\tOR  \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x0E000 ... 0x0EFFF :
                    fprintf ( outfile, "\tXOR \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x0F000 ... 0x0FFFF :
                    fprintf ( outfile, "\tXOR \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x12000 ... 0x12FFF :
                    fprintf ( outfile, "\tTEST\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x13000 ... 0x13FFF :
                    fprintf ( outfile, "\tTEST\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x18000 ... 0x18FFF :
                    fprintf ( outfile, "\tADD \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x19000 ... 0x19FFF :
                    fprintf ( outfile, "\tADD \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x1A000 ... 0x1AFFF :
                    fprintf ( outfile, "\tADDC\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x1B000 ... 0x1BFFF :
                    fprintf ( outfile, "\tADDC\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x1C000 ... 0x1CFFF :
                    fprintf ( outfile, "\tSUB \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x1D000 ... 0x1DFFF :
                    fprintf ( outfile, "\tSUB \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x1E000 ... 0x1EFFF :
                    fprintf ( outfile, "\tSUBC\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x1F000 ... 0x1FFFF :
                    fprintf ( outfile, "\tSUBC\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x14000 ... 0x14FFF :
                    fprintf ( outfile, "\tCOMP\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x15000 ... 0x15FFF :
                    fprintf ( outfile, "\tCOMP\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x20000 ... 0x20FFF :
                    switch ( c & 0xF ) {
                    case 0x2 :
                        fprintf ( outfile, "\tRL  \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                        break ;
                    case 0x6 :
                        fprintf ( outfile, "\tSL0 \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                        break ;
                    case 0x7 :
                        fprintf ( outfile, "\tSL1 \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                        break ;
                    case 0x0 :
                        fprintf ( outfile, "\tSLA \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                        break ;
                    case 0x4 :
                        fprintf ( outfile, "\tSLX \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                        break ;

                    case 0xC :
                        fprintf ( outfile, "\tRR  \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                        break ;
                    case 0xE :
                        fprintf ( outfile, "\tSR0 \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                        break ;
                    case 0xF :
                        fprintf ( outfile, "\tSR1 \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                        break ;
                    case 0x8 :
                        fprintf ( outfile, "\tSRA \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                        break ;
                    case 0xA :
                        fprintf ( outfile, "\tSRX \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                        break ;

                    default :
                        fprintf ( outfile, "\tINST\t0x%05X\t; %03X : %05X\n", c, pc, c ) ;
                    }
                    break ;

                case 0x34000 ... 0x34FFF :
                    fprintf ( outfile, "\tJUMP\t0x%03X   \t; %03X : %05X\n", Address10 ( c ), pc, c ) ;
                    break ;
                case 0x35000 ... 0x35FFF :
                    fprintf ( outfile, "\tJUMP\t%s, 0x%03X\t; %03X : %05X\n", Condition ( c ), Address10 ( c ), pc, c ) ;
                    break ;

                case 0x30000 ... 0x30FFF :
                    fprintf ( outfile, "\tCALL\t0x%03X   \t; %03X : %05X\n", Address10 ( c ), pc, c ) ;
                    break ;
                case 0x31000 ... 0x31FFF :
                    fprintf ( outfile, "\tCALL\t%s, 0x%03X\t; %03X : %05X\n", Condition ( c ), Address10 ( c ), pc, c ) ;
                    break ;

                case 0x2A000 ... 0x2AFFF :
                    fprintf ( outfile, "\tRET \t         \t; %03X : %05X\n", pc, c ) ;
                    break ;
                case 0x2B000 ... 0x2BFFF :
                    fprintf ( outfile, "\tRET \t%s       \t; %03X : %05X\n", Condition ( c ), pc, c ) ;
                    break ;

                case 0x2E000 ... 0x2EFFF :
                    fprintf ( outfile, "\tST  \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x2F000 ... 0x2FFFF :
                    fprintf ( outfile, "\tST  \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x06000 ... 0x06FFF :
                    fprintf ( outfile, "\tLD  \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x07000 ... 0x07FFF :
                    fprintf ( outfile, "\tLD  \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x2C000 ... 0x2CFFF :
                    fprintf ( outfile, "\tOUT \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x2D000 ... 0x2DFFF :
                    fprintf ( outfile, "\tOUT \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x04000 ... 0x04FFF :
                    fprintf ( outfile, "\tIN  \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x05000 ... 0x05FFF :
                    fprintf ( outfile, "\tIN  \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x3C000 :
                    fprintf ( outfile, "\tDINT\t \t; %03X : %05X\n", pc, c ) ;
                    break ;
                case 0x3C001 :
                    fprintf ( outfile, "\tEINT\t \t; %03X : %05X\n", pc, c ) ;
                    break ;
                case 0x38000 :
                    fprintf ( outfile, "\tRETI\tDISABLE\t; %03X : %05X\n", pc, c ) ;
                    break ;
                case 0x38001 :
                    fprintf ( outfile, "\tRETI\tENABLE\t; %03X : %05X\n", pc, c ) ;
                    break ;

                default :
                    fprintf ( outfile, "\tINST\t0x%05X\t; %03X : %05X\n", c, pc, c ) ;
                }
                pc += 1 ;
            } else
                state = stIDLE ;
            break ;
        case stDATA :
            if ( c != 0 ) {
                fprintf ( outfile, "\t.BYT\t0x%.2X, 0x%.2X\t; %03X : %05X\n", c & 0xFF, ( c >> 8 ) & 0xFF, pc, c ) ;
                pc += 1 ;
            } else
                state = stIDLE ;
            break ;
        }
    }
    fclose ( outfile ) ;
    return true ;
}

static bool writePSM6 ( const char * strPSMfile ) {
    FILE * outfile = NULL ;
    int pc = 0 ;
    uint32_t c = 0 ;
    enum {
        stIDLE, stCODE, stDATA
    } state = stIDLE ;

    outfile = fopen ( strPSMfile, "w" ) ;
    if ( outfile == NULL ) {
        fprintf ( stderr, "? Unable to open output file '%s'", strPSMfile ) ;
        return false ;
    }
    for ( pc = 0 ; pc < MEMSIZE ; ) {
        c = Code[ pc ] & 0x3FFFF ;
        switch ( state ) {
        case stIDLE :
            if ( c != 0 ) {
                switch ( pc ) {
                case 0x380 :
                    fprintf ( outfile, "\n\t.SCR\t0x%03X\n", pc ) ;
                    state = stDATA ;
                    break ;
                default :
                    fprintf ( outfile, "\n\t.ORG\t0x%03X\n", pc ) ;
                    state = stCODE ;
                }
            } else
                pc += 1 ;
            break ;
        case stCODE :
            if ( c != 0 ) {
                switch ( c ) {
                case 0x00000 ... 0x00FFF :
                    fprintf ( outfile, "\tMOVE\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x01000 ... 0x01FFF :
                    fprintf ( outfile, "\tMOVE\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x16000 ... 0x16FFF :
                    fprintf ( outfile, "\tSTAR\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x02000 ... 0x02FFF :
                    fprintf ( outfile, "\tAND \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x03000 ... 0x03FFF :
                    fprintf ( outfile, "\tAND \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x04000 ... 0x04FFF :
                    fprintf ( outfile, "\tOR  \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x05000 ... 0x05FFF :
                    fprintf ( outfile, "\tOR  \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x06000 ... 0x06FFF :
                    fprintf ( outfile, "\tXOR \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x07000 ... 0x07FFF :
                    fprintf ( outfile, "\tXOR \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x0C000 ... 0x0CFFF :
                    fprintf ( outfile, "\tTEST\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x0D000 ... 0x0DFFF :
                    fprintf ( outfile, "\tTEST\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x0E000 ... 0x0EFFF :
                    fprintf ( outfile, "\tTSTC\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x0F000 ... 0x0FFFF :
                    fprintf ( outfile, "\tTSTC\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x10000 ... 0x10FFF :
                    fprintf ( outfile, "\tADD \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x11000 ... 0x11FFF :
                    fprintf ( outfile, "\tADD \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x12000 ... 0x12FFF :
                    fprintf ( outfile, "\tADDC\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x13000 ... 0x13FFF :
                    fprintf ( outfile, "\tADDC\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x18000 ... 0x18FFF :
                    fprintf ( outfile, "\tSUB \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x19000 ... 0x19FFF :
                    fprintf ( outfile, "\tSUB \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x1A000 ... 0x1AFFF :
                    fprintf ( outfile, "\tSUBC\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x1B000 ... 0x1BFFF :
                    fprintf ( outfile, "\tSUBC\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x1C000 ... 0x1CFFF :
                    fprintf ( outfile, "\tCOMP\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x1D000 ... 0x1DFFF :
                    fprintf ( outfile, "\tCOMP\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x1E000 ... 0x1EFFF :
                    fprintf ( outfile, "\tCMPC\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x1F000 ... 0x1FFFF :
                    fprintf ( outfile, "\tCMPC\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x14000 ... 0x14FFF :
                    if ( c & 0xF0 ) {
                        fprintf ( outfile, "\tCORE\ts%X   \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                    } else
                        switch ( c & 0xF ) {
                        case 0x2 :
                            fprintf ( outfile, "\tRL  \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                            break ;
                        case 0x6 :
                            fprintf ( outfile, "\tSL0 \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                            break ;
                        case 0x7 :
                            fprintf ( outfile, "\tSL1 \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                            break ;
                        case 0x0 :
                            fprintf ( outfile, "\tSLA \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                            break ;
                        case 0x4 :
                            fprintf ( outfile, "\tSLX \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                            break ;

                        case 0xC :
                            fprintf ( outfile, "\tRR  \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                            break ;
                        case 0xE :
                            fprintf ( outfile, "\tSR0 \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                            break ;
                        case 0xF :
                            fprintf ( outfile, "\tSR1 \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                            break ;
                        case 0x8 :
                            fprintf ( outfile, "\tSRA \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                            break ;
                        case 0xA :
                            fprintf ( outfile, "\tSRX \ts%X      \t; %03X : %05X\n", DestReg ( c ), pc, c ) ;
                            break ;

                        default :
                            fprintf ( outfile, "\tINST\t0x%05X\t; %03X : %05X\n", c, pc, c ) ;
                        }
                    break ;

                case 0x22000 ... 0x22FFF :
                    fprintf ( outfile, "\tJUMP\t0x%03X      \t; %03X : %05X\n\n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x32000 ... 0x32FFF :
                    fprintf ( outfile, "\tJUMP\tZ, 0x%03X\t; %03X : %05X\n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x36000 ... 0x36FFF :
                    fprintf ( outfile, "\tJUMP\tNZ, 0x%03X\t; %03X : %05X\n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x3A000 ... 0x3AFFF :
                    fprintf ( outfile, "\tJUMP\tC, 0x%03X\t; %03X : %05X\n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x3E000 ... 0x3EFFF :
                    fprintf ( outfile, "\tJUMP\tNC, 0x%03X\t; %03X : %05X\n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x26000 ... 0x26FFF :
                    fprintf ( outfile, "\tJUMP\ts%X, s%X  \t; %03X : %05X\n\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x20000 ... 0x20FFF :
                    fprintf ( outfile, "\tCALL\t0x%03X      \t; %03X : %05X\n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x30000 ... 0x30FFF :
                    fprintf ( outfile, "\tCALL\tZ, 0x%03X\t; %03X : %05X\n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x34000 ... 0x34FFF :
                    fprintf ( outfile, "\tCALL\tNZ, 0x%03X\t; %03X : %05X\n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x38000 ... 0x38FFF :
                    fprintf ( outfile, "\tCALL\tC, 0x%03X\t; %03X : %05X\n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x3C000 ... 0x3CFFF :
                    fprintf ( outfile, "\tCALL\tNC, 0x%03X\t; %03X : %05X\n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x24000 ... 0x24FFF :
                    fprintf ( outfile, "\tCALL\ts%X, s%X  \t; %03X : %05X  \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x25000 ... 0x25FFF :
                    fprintf ( outfile, "\tRET \t         \t; %03X : %05X\n\n", pc, c ) ;
                    break ;
                case 0x31000 ... 0x31FFF :
                    fprintf ( outfile, "\tRET \t %s        \t; %03X : %05X\n", "Z", pc, c ) ;
                    break ;
                case 0x35000 ... 0x35FFF :
                    fprintf ( outfile, "\tRET \t %s       \t; %03X : %05X\n", "NZ", pc, c ) ;
                    break ;
                case 0x39000 ... 0x39FFF :
                    fprintf ( outfile, "\tRET \t %s        \t; %03X : %05X\n", "C", pc, c ) ;
                    break ;
                case 0x3D000 ... 0x3DFFF :
                    fprintf ( outfile, "\tRET \t %s       \t; %03X : %05X\n", "NC", pc, c ) ;
                    break ;
                case 0x21000 ... 0x21FFF :
                    fprintf ( outfile, "\tRET \ts%X, 0x%02X\t; %03X : %05X\n\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;


                case 0x2E000 ... 0x2EFFF :
                    fprintf ( outfile, "\tST  \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x2F000 ... 0x2FFFF :
                    fprintf ( outfile, "\tST  \ts%X, 0x%02X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x0A000 ... 0x0AFFF :
                    fprintf ( outfile, "\tLD  \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x0B000 ... 0x0BFFF :
                    fprintf ( outfile, "\tLD  \ts%X, 0x%02X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x2C000 ... 0x2CFFF :
                    fprintf ( outfile, "\tOUT \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x2D000 ... 0x2DFFF :
                    fprintf ( outfile, "\tOUT \ts%X, 0x%02X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x2B000 ... 0x2BFFF :
                    fprintf ( outfile, "\tOUTK\t0x%02X, 0x%X\t; %03X : %05X\n", ( c >> 4 ) & 0xFF, c & 0xF, pc, c ) ;
                    break ;

                case 0x08000 ... 0x08FFF :
                    fprintf ( outfile, "\tIN  \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x09000 ... 0x09FFF :
                    fprintf ( outfile, "\tIN  \ts%X, 0x%02X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x28000 :
                    fprintf ( outfile, "\tDINT\t \t; %03X : %05X\n", pc, c ) ;
                    break ;
                case 0x28001 :
                    fprintf ( outfile, "\tEINT\t \t; %03X : %05X\n", pc, c ) ;
                    break ;
                case 0x29000 :
                    fprintf ( outfile, "\tRETI\tDISABLE\t; %03X : %05X\n", pc, c ) ;
                    break ;
                case 0x29001 :
                    fprintf ( outfile, "\tRETI\tENABLE\t; %03X : %05X\n", pc, c ) ;
                    break ;

                case 0x37000 :
                    fprintf ( outfile, "\tBANK\tA\t; %03X : %05X\n", pc, c ) ;
                    break ;
                case 0x37001 :
                    fprintf ( outfile, "\tBANK\tB\t; %03X : %05X\n", pc, c ) ;
                    break ;

                default :
                    fprintf ( outfile, "\tINST\t0x%05X\t; %03X : %05X\n", pc, c, c ) ;
                }
                pc += 1 ;
            } else
                state = stIDLE ;
            break ;
        case stDATA :
            if ( c != 0 ) {
                fprintf ( outfile, "\t.BYT\t0x%02X, 0x%02X\t; %03X : %05X\n", c & 0xFF, ( c >> 8 ) & 0xFF, pc, c ) ;
                pc += 1 ;
            } else
                state = stIDLE ;
            break ;
        }
    }
    fclose ( outfile ) ;
    return true ;
}

static bool writeVHD6 ( const char * strPSMfile ) {
    FILE * outfile = NULL ;
    int pc = 0 ;
    uint32_t c = 0 ;

    outfile = fopen ( strPSMfile, "w" ) ;
    if ( outfile == NULL ) {
        fprintf ( stderr, " ? Unable to open output file '%s'", strPSMfile ) ;
        return false ;
    }

    fprintf ( outfile,
              "\n"
              "library ieee; \n"
              "use ieee.std_logic_1164.all; \n"
              "use ieee.numeric_std.all; \n"

              "\n"
              "use work.pb2_pkg.all ; \n"

              "\n"
              "entity PicoCore is\n"
              "\tport ( \n"
              "\t\tclk : in std_logic ; \n"
              "\t\trst : in std_logic ; \n"

              "\n"
              "\t\tPB2I : out t_PB2I ; \n"
              "\t\tPB2O : in t_PB2O ; \n"

              "\n"
              "\t\tPB2_IRQ : in std_logic ; \n"
              "\t\tPB2_IQA : out std_logic\n"
              "\t ) ; \n"
              "end PicoCore ; \n"

              "\n"
              "architecture mix of PicoCore is\n"
              "\t-- types\n"
              "\tsubtype PC_t is integer range 0 to 1023 ; \n"

              "\n"
              "\tsubtype REG_e is integer range 0 to 15 ; \n"
              "\tsubtype REG_t is std_logic_vector ( 7 downto 0 ) ; \n"
              "\ttype REG_BANK_t is array ( REG_e ) of REG_t ; \n"

              "\n"
              "\tsubtype SP_e is integer range 0 to 31 ; \n"
              "\ttype STACK_e is\n"
              "\t\trecord\n"
              "\t\t\tpc : PC_t ; \n"
              "\t\t\tc : std_logic ; \n"
              "\t\t\tz : boolean ; \n"
              "\t\tend record ; \n"
              "\ttype STACK_t is array ( SP_e ) of STACK_e ; \n"

              "\n"
              "\t-- state\n"
              "\tsignal pc : PC_t ; \n"
              "\tsignal sp : SP_e ; \n"
              "\tsignal stack : STACK_t ; \n"
              "\tsignal s : REG_BANK_t ; \n"
              "\tsignal t, z, i : boolean ; \n"
              "\tsignal c : std_logic ; \n"

              "\n"
              "\t-- scratchpad\n"
              "\tsignal scratch_ram_addr : std_logic_vector ( 7 downto 0 ) ; \n"
              "\tsignal scratch_ram_di : std_logic_vector ( 7 downto 0 ) ; \n"
              "\tsignal scratch_ram_do : std_logic_vector ( 7 downto 0 ) ; \n"
              "\tsignal nscr_we : std_logic := '0' ; \n"

              "\n"
              "\t-- data and address paths\n"
              "\tsignal sx_addr : std_logic_vector ( 3 downto 0 ) ; \n"
              "\tsignal sy_addr : std_logic_vector ( 3 downto 0 ) ; \n"
              "\tsignal sx : std_logic_vector ( 7 downto 0 ) ; \n"
              "\tsignal sy : std_logic_vector ( 7 downto 0 ) ; \n"
              "\tsignal ad : std_logic_vector ( 7 downto 0 ) ; \n"
              "\tsignal nreg_we : std_logic := '0' ; \n"

              "\n"
              "\tsignal npc : PC_t ; \n"
              "\tsignal nsp : SP_e ; \n"
              "\tsignal alu : std_logic_vector ( 7 downto 0 ) ; \n"
              "\tsignal nc : std_logic ; \n"
              "\tsignal nz : boolean ; \n"
              "\tsignal ni : boolean ; \n"

              "\n"
              "\tsignal nrd : std_logic ; \n"
              "\tsignal nwr : std_logic ; \n"

              "begin\n"

              "\tPB2I.ck <= clk ; \n"
              "\tPB2I.rs <= rst ; \n"
              "\tPB2I.da <= sx ; \n"
              "\tPB2I.ad <= ad ; \n"

              "\n"
              "\trb : block is\n"
              "\t\ttype ram_type is array ( 15 downto 0 ) of std_logic_vector ( 7 downto 0 ) ; \n"
              "\t\tsignal register_ram : ram_type ; \n"
              "\tbegin\n"
              "\t\tprocess ( clk ) is\n"
              "\t\tbegin\n"
              "\t\t\tif rising_edge ( clk ) then\n"
              "\t\t\t\tif nreg_we = '1' then\n"
              "\t\t\t\t\tregister_ram ( to_integer ( unsigned ( sx_addr ) ) ) <= alu ; \n"
              "\t\t\t\tend if ; \n"
              "\t\tend if ; \n"
              "\t\tend process ; \n"

              "\n"
              "\t\tsx <= register_ram ( to_integer ( unsigned ( sx_addr ) ) ) ; \n"
              "\t\tsy <= register_ram ( to_integer ( unsigned ( sy_addr ) ) ) ; \n"
              "\tend block ; \n"

              "\n"
              "\tsb : block is \n"
              "\t\ttype scratch_type is array ( 255 downto 0 ) of std_logic_vector ( 7 downto 0 ) ; \n"
              "\t\tsignal scratch_ram : scratch_type ; \n"
              "\tbegin \n"
              "\t\tprocess ( clk ) is \n"
              "\t\tbegin \n"
              "\t\t\tif rising_edge ( clk ) then \n"
              "\t\t\t\tif nscr_we = '1' then \n"
              "\t\t\t\t\tscratch_ram ( to_integer ( unsigned ( scratch_ram_addr ) ) ) <= scratch_ram_di ; \n"
              "\t\t\t\tend if ; \n"
              "\t\t\tend if ; \n"
              "\t\tend process ; \n"
              "\n"
              "\t\tscratch_ram_do <= scratch_ram ( to_integer ( unsigned ( scratch_ram_addr ) ) ) ; \n"
              "\tend block ; \n"

              "\n"
              "\tprocess ( rst, clk ) is\n"
              "\tbegin\n"
              "\t\tif rst = '1' then\n"
              "\t\t\tPB2I.wr <= '0' ; \n"
              "\t\t\tPB2I.rd <= '0' ; \n"

              "\n"
              "\t\t\tpc <= 0 ; \n"
              "\t\t\tsp <= 0 ; \n"
              "\t\t\tc <= '0' ; \n"
              "\t\t\tz <= false ; \n"
              "\t\t\ti <= false ; \n"
              "\t\t\tt <= false ; \n"
              "\t\telsif rising_edge( clk ) then\n"
              "\t\t\tt <= not t ; \n"

              "\n"
              "\t\t\tif t then\n"
              "\t\t\t\tstack( sp ) <= ( pc => npc, c => c, z => z ) ; \n"
              "\t\t\t\tc <= nc ; \n"
              "\t\t\t\tz <= nz ; \n"
              "\t\t\t\ti <= ni ; \n"
              "\t\t\tend if ; \n"

              "\n"
              "\t\t\tif t then\n"
              "\t\t\t\tPB2I.rd <= nrd ; \n"
              "\t\t\t\tPB2I.wr <= nwr ; \n"
              "\t\t\tend if ; \n"

              "\n"
              "\t\t\tif not t then\n"
              "\t\t\t\tpc <= npc ; \n"
              "\t\t\t\tsp <= nsp ; \n"
              "\t\t\tend if ; \n"

              "\n"
              "\t\tend if ; \n"
              "\tend process ; \n"
              "\n" );

    fprintf ( outfile,
              "\tprocess ( t, pc, sp, stack, alu, PB2O, sx, sy, c, z, i ) is\n"
              "\t\tvariable sx9 : unsigned( 8 downto 0 ) ; \n"
              "\t\tvariable adder : unsigned ( 8 downto 0 ) ; \n"
              "\tbegin\n"
              "\t\t\tnpc <= pc + 1 ; \n"
              "\t\t\tnsp <= sp ; \n"

              "\n"
              "\t\t\tadder := c & X\"00\" ; \n"

              "\n"
              "\t\t\tnz <= z ; \n"
              "\t\t\tni <= i ; \n"

              "\n"
              "\t\t\tnrd <= '0' ; \n"
              "\t\t\tnwr <= '0' ; \n"
              "\t\t\tnscr_we <= '0' ; \n"
              "\t\t\tnreg_we <= '0' ; \n"
              "\n"
            ) ;

    fprintf ( outfile,
              "\t\t\tsx9 := '0' & unsigned( sx ) ; \n"
              "\n"
              "\t\t\t-- program\n"
              "\t\t\tcase ( pc ) is\n"
            ) ;

    for ( pc = 0 ; pc < 1024 ; ) {
        c = Code[ pc ] & 0x3FFFF ;
        if ( c != 0 )
            fprintf ( outfile, "\t\t\twhen %d =>\n", pc ) ;
        switch ( c ) {
        case 0x00000 :
            break ;
        case 0x00001 ... 0x00FFF :
            fprintf ( outfile, "\t\t\t\t-- MOVE\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tsy_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tnreg_we <= '1' ; \n",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;
        case 0x01000 ... 0x01FFF :
            fprintf ( outfile, "\t\t\t\t-- MOVE\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\talu <= X\"%02X\" ; \n"
                      "\t\t\t\tnreg_we <= '1' ; \n",
                      DestReg ( c ), Constant ( c ) ) ;
            break ;
        case 0x16000 ... 0x16FFF :
            fprintf ( outfile, "\t\t\t\t-- STAR\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile, "\t\t\t\tassert false ; \n" ) ;
            break ;

        case 0x02000 ... 0x02FFF :
            fprintf ( outfile, "\t\t\t\t-- AND \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tsy_addr <= X\"%01X\" ; \n"
                      "\t\t\t\talu <= sx and sy ; \n"
                      "\t\t\t\tnreg_we <= '1' ; \n",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;
        case 0x03000 ... 0x03FFF :
            fprintf ( outfile, "\t\t\t\t-- AND \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\talu <= sx and X\"%02X\" ; \n"
                      "\t\t\t\tnreg_we <= '1' ; \n",
                      DestReg ( c ), Constant ( c ) ) ;
            break ;

        case 0x04000 ... 0x04FFF :
            fprintf ( outfile, "\t\t\t\t-- OR  \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tsy_addr <= X\"%01X\" ; \n"
                      "\t\t\t\talu <= sx or sy ; \n"
                      "\t\t\t\tnreg_we <= '1' ; \n",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;
        case 0x05000 ... 0x05FFF :
            fprintf ( outfile, "\t\t\t\t-- OR  \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\talu <= sx or X\"%02X\" ; \n"
                      "\t\t\t\tnreg_we <= '1' ; \n",
                      DestReg ( c ), Constant ( c ) ) ;
            break ;

        case 0x06000 ... 0x06FFF :
            fprintf ( outfile, "\t\t\t\t-- XOR \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tsy_addr <= X\"%01X\" ; \n"
                      "\t\t\t\talu <= sx xor sy ; \n"
                      "\t\t\t\tnreg_we <= '1' ; \n",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;
        case 0x07000 ... 0x07FFF :
            fprintf ( outfile, "\t\t\t\t-- XOR \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\talu <= sx xor X\"%02X\" ; \n"
                      "\t\t\t\tnreg_we <= '1' ; \n",
                      DestReg ( c ), Constant ( c ) ) ;
            break ;

        case 0x0C000 ... 0x0CFFF :
            fprintf ( outfile, "\t\t\t\t-- TEST\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tsy_addr <= X\"%01X\" ; \n"
                      "\t\t\t\talu <= sx and sy ; \n",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;
        case 0x0D000 ... 0x0DFFF :
            fprintf ( outfile, "\t\t\t\t-- TEST\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\talu <= sx and X\"%02X\" ; \n",
                      DestReg ( c ), Constant ( c ) ) ;
            break ;
        case 0x0E000 ... 0x0EFFF :
            fprintf ( outfile, "\t\t\t\t-- TSTC\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tsy_addr <= X\"%01X\" ; \n"
                      "\t\t\t\talu <= sx and sy ; \n",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;
        case 0x0F000 ... 0x0FFFF :
            fprintf ( outfile, "\t\t\t\t-- TSTC\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\talu <= sx and X\"%02X\" ; \n",
                      DestReg ( c ), Constant ( c ) ) ;
            break ;

        case 0x10000 ... 0x10FFF :
            fprintf ( outfile, "\t\t\t\t-- ADD \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tsy_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tadder := sx9 + sy9 ; \n"
                      "\t\t\t\talu <= std_logic_vector( adder( 7 downto 0 ) ) ; \n"
                      "\t\t\t\tnc <= adder( 8 ) ; \n"
                      "\t\t\t\tnz <= alu = X\"00\" ; \n"
                      "\t\t\t\tnreg_we <= '1' ;",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;
        case 0x11000 ... 0x11FFF :
            fprintf ( outfile, "\t\t\t\t-- ADD \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tadder := sx9 + X\"%02X\" ; \n"
                      "\t\t\t\talu <= std_logic_vector( adder( 7 downto 0 ) ) ; \n"
                      "\t\t\t\tnc <= adder( 8 ) ; \n"
                      "\t\t\t\tnz <= alu = X\"00\" ; \n"
                      "\t\t\t\tnreg_we <= '1' ; \n",
                      DestReg ( c ), Constant ( c ) ) ;
            break ;

        case 0x12000 ... 0x12FFF :
            fprintf ( outfile, "\t\t\t\t-- ADDC\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tsy_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tadder := sx9 + sy + c ; \n"
                      "\t\t\t\talu <= std_logic_vector( adder( 7 downto 0 ) ) ; \n"
                      "\t\t\t\tnc <= adder( 8 ) ; \n"
                      "\t\t\t\tnz <= alu = X\"00\" ; \n"
                      "\t\t\t\tnreg_we <= '1' ;",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;
        case 0x13000 ... 0x13FFF :
            fprintf ( outfile, "\t\t\t\t-- ADDC\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tadder := sx9 + X\"%02X\" + c ; \n"
                      "\t\t\t\talu <= std_logic_vector( adder( 7 downto 0 ) ) ; \n"
                      "\t\t\t\tnc <= adder( 8 ) ; \n"
                      "\t\t\t\tnz <= alu = X\"00\" ; \n"
                      "\t\t\t\tnreg_we <= '1' ; \n",
                      DestReg ( c ), Constant ( c ) ) ;
            break ;

        case 0x18000 ... 0x18FFF :
            fprintf ( outfile, "\t\t\t\t-- SUB \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tsy_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tadder := sx9 - sy ; \n"
                      "\t\t\t\talu <= std_logic_vector( adder( 7 downto 0 ) ) ; \n"
                      "\t\t\t\tnc <= adder( 8 ) ; \n"
                      "\t\t\t\tnz <= alu = X\"00\" ; \n"
                      "\t\t\t\tnreg_we <= '1' ;",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;
        case 0x19000 ... 0x19FFF :
            fprintf ( outfile, "\t\t\t\t-- SUB \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tadder := sx9 - X\"%02X\" ; \n"
                      "\t\t\t\talu <= std_logic_vector( adder( 7 downto 0 ) ) ; \n"
                      "\t\t\t\tnc <= adder( 8 ) ; \n"
                      "\t\t\t\tnz <= alu = X\"00\" ; \n"
                      "\t\t\t\tnreg_we <= '1' ; \n",
                      DestReg ( c ), Constant ( c ) ) ;
            break ;

        case 0x1A000 ... 0x1AFFF :
            fprintf ( outfile, "\t\t\t\t-- SUBC\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tsy_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tadder := sx9 - sy - c ; \n"
                      "\t\t\t\talu <= std_logic_vector( adder( 7 downto 0 ) ) ; \n"
                      "\t\t\t\tnc <= adder( 8 ) ; \n"
                      "\t\t\t\tnz <= alu = X\"00\" ; \n"
                      "\t\t\t\tnreg_we <= '1' ;",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;
        case 0x1B000 ... 0x1BFFF :
            fprintf ( outfile, "\t\t\t\t-- SUBC\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tadder := sx9 - X\"%02X\" - c ; \n"
                      "\t\t\t\talu <= std_logic_vector( adder( 7 downto 0 ) ) ; \n"
                      "\t\t\t\tnc <= adder( 8 ) ; \n"
                      "\t\t\t\tnz <= alu = X\"00\" ; \n"
                      "\t\t\t\tnreg_we <= '1' ; \n",
                      DestReg ( c ), Constant ( c ) ) ;
            break ;

        case 0x1C000 ... 0x1CFFF :
            fprintf ( outfile, "\t\t\t\t-- COMP\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tsy_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tadder := sx9 - sy ; \n"
                      "\t\t\t\talu <= std_logic_vector( adder( 7 downto 0 ) ) ; \n"
                      "\t\t\t\tnc <= adder( 8 ) ; \n"
                      "\t\t\t\tnz <= alu = X\"00\" ; \n",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;
        case 0x1D000 ... 0x1DFFF :
            fprintf ( outfile, "\t\t\t\t-- COMP\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tadder := sx9 - X\"%02X\" ; \n"
                      "\t\t\t\talu <= std_logic_vector( adder( 7 downto 0 ) ) ; \n"
                      "\t\t\t\tnc <= adder( 8 ) ; \n"
                      "\t\t\t\tnz <= alu = X\"00\" ; \n",
                      DestReg ( c ), Constant ( c ) ) ;
            break ;
        case 0x1E000 ... 0x1EFFF :
            fprintf ( outfile, "\t\t\t\t-- CMPC\ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tsy_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tadder := sx9 - sy - c ; \n"
                      "\t\t\t\talu <= std_logic_vector( adder( 7 downto 0 ) ) ; \n"
                      "\t\t\t\tnc <= adder( 8 ) ; \n"
                      "\t\t\t\tnz <= alu = X\"00\" ; \n",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;
        case 0x1F000 ... 0x1FFFF :
            fprintf ( outfile, "\t\t\t\t-- CMPC\ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tadder := sx9 - X\"%02X\" - c ; \n"
                      "\t\t\t\talu <= std_logic_vector( adder( 7 downto 0 ) ) ; \n"
                      "\t\t\t\tnc <= adder( 8 ) ; \n"
                      "\t\t\t\tnz <= alu = X\"00\" ; \n",
                      DestReg ( c ), Constant ( c ) ) ;
            break ;

        case 0x14000 ... 0x14FFF :
            if ( c & 0xF0 ) {
                fprintf ( outfile, "\t\t\t\t-- CORE\ts%X   \t; %03X : %05X\n",
                          DestReg ( c ), pc, c ) ;
            } else
                switch ( c & 0xF ) {
                case 0x2 :
                    fprintf ( outfile, "\t\t\t\t-- RL  \ts%X      \t; %03X : %05X\n",                             DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                              "\t\t\t\talu <= sx( 6 downto 0 ) & sx( 7 ) ; \n"
                              "\t\t\t\tnc <= sx( 7 ) ; \n"
                              "\t\t\t\tnreg_we <= '1' ; \n",
                              DestReg ( c ) ) ;
                    break ;
                case 0x6 :
                    fprintf ( outfile, "\t\t\t\t-- SL0 \ts%X      \t; %03X : %05X\n",                             DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                              "\t\t\t\talu <= sx( 6 downto 0 ) & '0' ; \n"
                              "\t\t\t\tnc <= sx( 7 ) ; \n"
                              "\t\t\t\tnreg_we <= '1' ; \n",
                              DestReg ( c ) ) ;
                    break ;
                case 0x7 :
                    fprintf ( outfile, "\t\t\t\t-- SL1 \ts%X      \t; %03X : %05X\n",                             DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                              "\t\t\t\talu <= sx( 6 downto 0 ) & '1' ; \n"
                              "\t\t\t\tnc <= sx( 7 ) ; \n"
                              "\t\t\t\tnreg_we <= '1' ; \n",
                              DestReg ( c ) ) ;
                    break ;
                case 0x0 :
                    fprintf ( outfile, "\t\t\t\t-- SLA \ts%X      \t; %03X : %05X\n",                             DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                              "\t\t\t\talu <= sx( 6 downto 0 ) & c ; \n"
                              "\t\t\t\tnc <= sx( 7 ) ; \n"
                              "\t\t\t\tnreg_we <= '1' ; \n",
                              DestReg ( c ) ) ;
                    break ;
                case 0x4 :
                    fprintf ( outfile, "\t\t\t\t-- SLX \ts%X      \t; %03X : %05X\n",                             DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                              "\t\t\t\talu <= sx( 6 downto 0 ) & sx( 0 ) ; \n"
                              "\t\t\t\tnc <= sx( 7 ) ; \n"
                              "\t\t\t\tnreg_we <= '1' ; \n",
                              DestReg ( c ) ) ;
                    break ;

                case 0xC :
                    fprintf ( outfile, "\t\t\t\t-- RR  \ts%X      \t; %03X : %05X\n",                             DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                              "\t\t\t\talu <= sx( 0 ) & sx( 7 downto 1 ) ; \n"
                              "\t\t\t\tnc <= sx( 0 ) ; \n"
                              "\t\t\t\tnreg_we <= '1' ; \n",
                              DestReg ( c ) ) ;
                    break ;
                case 0xE :
                    fprintf ( outfile, "\t\t\t\t-- SR0 \ts%X      \t; %03X : %05X\n",                             DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                              "\t\t\t\talu <= '0' & sx( 7 downto 1 ) ; \n"
                              "\t\t\t\tnc <= sx( 0 ) ; \n"
                              "\t\t\t\tnreg_we <= '1' ; \n",
                              DestReg ( c ) ) ;
                    break ;
                case 0xF :
                    fprintf ( outfile, "\t\t\t\t-- SR1 \ts%X      \t; %03X : %05X\n",                             DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                              "\t\t\t\talu <= '1' & sx( 7 downto 1 ) ; \n"
                              "\t\t\t\tnc <= sx( 0 ) ; \n"
                              "\t\t\t\tnreg_we <= '1' ; \n",
                              DestReg ( c ) ) ;
                    break ;
                case 0x8 :
                    fprintf ( outfile, "\t\t\t\t-- SRA \ts%X      \t; %03X : %05X\n",                             DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                              "\t\t\t\talu <= c & sx( 7 downto 1 ) ; \n"
                              "\t\t\t\tnc <= sx( 0 ) ; \n"
                              "\t\t\t\tnreg_we <= '1' ; \n",
                              DestReg ( c ) ) ;
                    break ;
                case 0xA :
                    fprintf ( outfile, "\t\t\t\t-- SRX \ts%X      \t; %03X : %05X\n",                             DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                              "\t\t\t\talu <= sx( 7 ) & sx( 7 downto 1 ) ; \n"
                              "\t\t\t\tnc <= sx( 0 ) ; \n"
                              "\t\t\t\tnreg_we <= '1' ; \n",
                              DestReg ( c ) ) ;
                    break ;

                default :
                    fprintf ( outfile, "\t\t\t\t-- INST\t0x%05X\t; %03X : %05X\n", c, pc, c ) ;
                    fprintf ( outfile, "\t\t\t\tassert false ; \n" ) ;
                }
            break ;

        case 0x22000 ... 0x22FFF :
            fprintf ( outfile, "\t\t\t\t-- JUMP\t0x%03X      \t; %03X : %05X\n", Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tnpc <= %d ; \n",
                      Address12 ( c ) ) ;
            break ;
        case 0x32000 ... 0x32FFF :
            fprintf ( outfile, "\t\t\t\t-- JUMP\tZ, 0x%03X\t; %03X : %05X\n", Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif z then \n"
                      "\t\t\t\t\tnpc <= %d ; \n"
                      "\t\t\t\tend if ; \n",
                      Address12 ( c ) ) ;
            break ;
        case 0x36000 ... 0x36FFF :
            fprintf ( outfile, "\t\t\t\t-- JUMP\tNZ, 0x%03X\t; %03X : %05X\n", Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif not z then \n"
                      "\t\t\t\t\tnpc <= %d ; \n"
                      "\t\t\t\tend if ; \n",
                      Address12 ( c ) ) ;
            break ;
        case 0x3A000 ... 0x3AFFF :
            fprintf ( outfile, "\t\t\t\t-- JUMP\tC, 0x%03X\t; %03X : %05X\n", Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif c = '1' then \n"
                      "\t\t\t\t\tnpc <= %d ; \n"
                      "\t\t\t\tend if ; \n",
                      Address12 ( c ) ) ;
            break ;
        case 0x3E000 ... 0x3EFFF :
            fprintf ( outfile, "\t\t\t\t-- JUMP\tNC, 0x%03X\t; %03X : %05X\n", Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif c = '0' then \n"
                      "\t\t\t\t\tnpc <= %d ; \n"
                      "\t\t\t\tend if ; \n",
                      Address12 ( c ) ) ;
            break ;
        case 0x26000 ... 0x26FFF :
            fprintf ( outfile, "\t\t\t\t-- JUMP\ts%X, s%X  \t; %03X : %05X\n\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx <= X\"%02X\" ; \n"
                      "\t\t\t\tsy <= X\"%02X\" ; \n"
                      "\t\t\t\tnpc <= sx & sy ; \n",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;

        case 0x20000 ... 0x20FFF :
            fprintf ( outfile, "\t\t\t\t-- CALL\t0x%03X      \t; %03X : %05X\n", Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tnsp <= sp + 1 ; \n"
                      "\t\t\t\tnpc <= %d ; \n",
                      Address12 ( c ) ) ;
            break ;
        case 0x30000 ... 0x30FFF :
            fprintf ( outfile, "\t\t\t\t-- CALL\tZ, 0x%03X\t; %03X : %05X\n",
                      Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif z then\n"
                      "\t\t\t\t\tnsp <= sp + 1 ; \n"
                      "\t\t\t\t\tnpc <= %d ; \n"
                      "\t\t\t\tend if ; \n",
                      Address12 ( c ) ) ;
            break ;
        case 0x34000 ... 0x34FFF :
            fprintf ( outfile, "\t\t\t\t-- CALL\tNZ, 0x%03X\t; %03X : %05X\n",
                      Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif not z then\n"
                      "\t\t\t\t\tnsp <= sp + 1 ; \n"
                      "\t\t\t\t\tnpc <= %d ; \n"
                      "\t\t\t\tend if ; \n",
                      Address12 ( c ) ) ;
            break ;
        case 0x38000 ... 0x38FFF :
            fprintf ( outfile, "\t\t\t\t-- CALL\tC, 0x%03X\t; %03X : %05X\n",
                      Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif c = '1' then\n"
                      "\t\t\t\t\tnsp <= sp + 1 ; \n"
                      "\t\t\t\t\tnpc <= %d ; \n"
                      "\t\t\t\tend if ; \n",
                      Address12 ( c ) ) ;
            break ;
        case 0x3C000 ... 0x3CFFF :
            fprintf ( outfile, "\t\t\t\t-- CALL\tNC, 0x%03X\t; %03X : %05X\n",
                      Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif c = '0' then\n"
                      "\t\t\t\t\tnsp <= sp + 1 ; \n"
                      "\t\t\t\t\tnpc <= %d ; \n"
                      "\t\t\t\tend if ; \n",
                      Address12 ( c ) ) ;
            break ;
        case 0x24000 ... 0x24FFF :
            fprintf ( outfile, "\t\t\t\t-- CALL\ts%X, s%X  \t; %03X : %05X  \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tnsp <= sp + 1 ; \n"
                      "\t\t\t\tnpc <= X\"%01X%02X\" ; \n",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;

        case 0x25000 ... 0x25FFF :
            fprintf ( outfile, "\t\t\t\t-- RET \t         \t; %03X : %05X\n", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tnpc <= stack( sp ).pc ; \n"
                      "\t\t\t\tnsp <= sp - 1 ; \n" ) ;
            break ;
        case 0x31000 ... 0x31FFF :
            fprintf ( outfile, "\t\t\t\t-- RET \t %s        \t; %03X : %05X\n", "Z", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif z then\n"
                      "\t\t\t\t\tnpc <= stack ( sp ).pc ; \n"
                      "\t\t\t\t\tnsp <= sp - 1 ; \n"
                      "\t\t\t\tend if ; \n" ) ;
            break ;
        case 0x35000 ... 0x35FFF :
            fprintf ( outfile, "\t\t\t\t-- RET \t %s       \t; %03X : %05X\n", "NZ", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif not z then\n"
                      "\t\t\t\t\tnpc <= stack ( sp ).pc ; \n"
                      "\t\t\t\t\tnsp <= sp - 1 ; \n"
                      "\t\t\t\tend if ; \n" ) ;
            break ;
        case 0x39000 ... 0x39FFF :
            fprintf ( outfile, "\t\t\t\t-- RET \t %s        \t; %03X : %05X\n", "C", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif c = '1' then\n"
                      "\t\t\t\t\tnpc <= stack ( sp ).pc ; \n"
                      "\t\t\t\t\tnsp <= sp - 1 ; \n"
                      "\t\t\t\tend if ; \n" ) ;
            break ;
        case 0x3D000 ... 0x3DFFF :
            fprintf ( outfile, "\t\t\t\t-- RET \t %s       \t; %03X : %05X\n", "NC", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif c = '0' then\n"
                      "\t\t\t\t\tnpc <= stack ( sp ).pc ; \n"
                      "\t\t\t\t\tnsp <= sp - 1 ; \n"
                      "\t\t\t\tend if ; \n" ) ;
            break ;
        case 0x21000 ... 0x21FFF :
            fprintf ( outfile, "\t\t\t\t-- RET \ts%X, 0x%02X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01X\" ; \n"
                      "\t\t\t\tnpc <= stack ( sp ).pc ; \n"
                      "\t\t\t\tnsp <= sp - 1 ; \n"
                      "\t\t\t\talu <= X\"%.2X\" ; \n",
                      DestReg ( c ), Constant ( c ) ) ;
            break ;


        case 0x2E000 ... 0x2EFFF :
            fprintf ( outfile, "\t\t\t\t-- ST  \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01x\" ; \n"
                      "\t\t\t\tsy_addr <= X\"%01x\" ; \n"
                      "\t\t\t\tscratch_ram_di <= sx ; \n"
                      "\t\t\t\tscratch_ram_addr <= sy ; \n"
                      "\t\t\t\tnscr_we <= '1' ; \n",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;
        case 0x2F000 ... 0x2FFFF :
            fprintf ( outfile, "\t\t\t\t-- ST  \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01x\" ; \n"
                      "\t\t\t\tscratch_ram_di <= sx ; \n"
                      "\t\t\t\tscratch_ram_addr <= X\"%02x\" ; \n"
                      "\t\t\t\tnscr_we <= '1' ; \n",
                      DestReg ( c ), Constant ( c ) ) ;
            break ;

        case 0x0A000 ... 0x0AFFF :
            fprintf ( outfile, "\t\t\t\t-- LD  \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01x\" ; \n"
                      "\t\t\t\tsy_addr <= X\"%01x\" ; \n"
                      "\t\t\t\tscratch_ram_addr <= sy ; \n"

                      "\t\t\t\tif t then\n"
                      "\t\t\t\tnrd <= '1' ; \n"
                      "\t\t\t\t\talu <= scratch_ram_do ; \n"
                      "\t\t\t\tnreg_we <= '1' ; \n"
                      "\t\t\t\tend if ; \n",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;
        case 0x0B000 ... 0x0BFFF :
            fprintf ( outfile, "\t\t\t\t-- LD  \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01x\" ; \n"
                      "\t\t\t\tscratch_ram_addr <= X\"%02x\" ; \n"

                      "\t\t\t\tif t then\n"
                      "\t\t\t\tnrd <= '1' ; \n"
                      "\t\t\t\talu <= scratch_ram_do ; \n"
                      "\t\t\t\tnreg_we <= '1' ; \n"
                      "\t\t\t\tend if ; \n",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;

        case 0x2C000 ... 0x2CFFF :
            fprintf ( outfile, "\t\t\t\t-- OUT \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01x\" ; \n"
                      "\t\t\t\tsy_addr <= X\"%01x\" ; \n"
                      "\t\t\t\tad <= sy ; \n"
                      "\t\t\t\tnwr <= '1' ; \n",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;
        case 0x2D000 ... 0x2DFFF :
            fprintf ( outfile, "\t\t\t\t-- OUT \ts%X, 0x%.2X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01x\" ; \n"
                      "\t\t\t\tad <= X\"%02x\" ; \n"
                      "\t\t\t\tnwr <= '1' ; \n",
                      DestReg ( c ), Constant ( c ) ) ;
            break ;
        case 0x2B000 ... 0x2BFFF :
            fprintf ( outfile, "\t\t\t\t-- OUTK\t0x%.2X, 0x%X\t; %03X : %05X\n", ( c >> 4 ) & 0xFF, c & 0xF, pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tad <= X\"%02x\" ; \n"
                      "\t\t\t\alu <= X\"%02x\" ; \n"
                      "\t\t\t\tPB2I.wr <= '1' ; \n",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;

        case 0x08000 ... 0x08FFF :
            fprintf ( outfile, "\t\t\t\t-- IN  \ts%X, s%X  \t; %03X : %05X\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01x\" ; \n"
                      "\t\t\t\tsy_addr <= X\"%01x\" ; \n"
                      "\t\t\t\tad <= sy ; \n"

                      "\t\t\t\tif t then\n"
                      "\t\t\t\tnrd <= '1' ; \n"
                      "\t\t\t\t\talu <= PB2O.da ; \n"
                      "\t\t\t\tnreg_we <= '1' ; \n"
                      "\t\t\t\tend if ; \n",
                      DestReg ( c ), SrcReg ( c ) ) ;
            break ;
        case 0x09000 ... 0x09FFF :
            fprintf ( outfile, "\t\t\t\t-- IN  \ts%X, 0x%02X\t; %03X : %05X\n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tsx_addr <= X\"%01x\" ; \n"
                      "\t\t\t\tad <= X\"%02x\" ; \n"

                      "\t\t\t\tif t then\n"
                      "\t\t\t\tnrd <= '1' ; \n"
                      "\t\t\t\talu <= PB2O.da ; \n"
                      "\t\t\t\tnreg_we <= '1' ; \n"
                      "\t\t\t\tend if ; \n",
                      DestReg ( c ), Constant ( c ) ) ;
            break ;

        case 0x28000 :
            fprintf ( outfile, "\t\t\t\t-- DINT\t \t; %03X : %05X\n", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tni <= false ; \n" ) ;
            break ;
        case 0x28001 :
            fprintf ( outfile, "\t\t\t\t-- EINT\t \t; %03X : %05X\n", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tni <= true ; \n" ) ;
            break ;
        case 0x29000 :
            fprintf ( outfile, "\t\t\t\t-- RETI\tDISABLE\t; %03X : %05X\n", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tni <= false ; \n"
                      "\t\t\t\tnpc <= stack( sp ).pc ; \n"
                      "\t\t\t\tnc <= stack( sp ).c ; \n"
                      "\t\t\t\tnz <= stack( sp ).z ; \n"
                      "\t\t\t\tnsp <= sp - 1 ; \n" ) ;
            break ;
        case 0x29001 :
            fprintf ( outfile, "\t\t\t\t-- RETI\tENABLE\t; %03X : %05X\n", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tni <= true ; \n"
                      "\t\t\t\tnpc <= stack( sp ).pc ; \n"
                      "\t\t\t\tnc <= stack( sp ).c ; \n"
                      "\t\t\t\tnz <= stack( sp ).z ; \n"
                      "\t\t\t\tnsp <= sp - 1 ; \n" ) ;
            break ;

        case 0x37000 :
            fprintf ( outfile, "\t\t\t\t-- BANK\tA\t; %03X : %05X\n", pc, c ) ;
            fprintf ( outfile, "\t\t\t\tassert false ; \n" ) ;
            break ;
        case 0x37001 :
            fprintf ( outfile, "\t\t\t\t-- BANK\tB\t; %03X : %05X\n", pc, c ) ;
            fprintf ( outfile, "\t\t\t\tassert false ; \n" ) ;
            break ;

        default :
            fprintf ( outfile, "\t\t\t\t-- INST\t0x%05X\t; %03X : %05X\n", pc, c, c ) ;
            fprintf ( outfile, "\t\t\t\tassert false ; \n" ) ;
        }
        pc += 1 ;
    }

    // postamble
    fprintf ( outfile, "\t\t\twhen others =>\n"
              "\t\t\tend case ; \n"
              "\n"
              "\tend process ; \n"
              "end mix ; \n"
            ) ;

    fclose ( outfile ) ;
    return true ;
}

int main ( int argc, char * argv[] ) {
    char code_filename[ 256 ] = { '\0' } ;
    char data_filename[ 256 ] = { '\0' } ;
    char psm_filename[ 256 ] = { '\0' } ;

    bool result = false ;
    bool bOptErr = false ;
    bool bKCPSM6 = false ;
    bool bVerbose = false ;
    bool bMEM = false ;
    bool bXDL = false ;
    bool bPico = false ;

    extern char * optarg ;
    extern int optind, optopt, opterr ;
    int i, optch ;

    opterr = -1 ;
    while ( ( optch = getopt ( argc, argv, "6c:d:hm:ps:vx" ) ) != -1 ) {
        switch ( optch ) {
        case '6' :
            bKCPSM6 = true ;
            if ( bVerbose )
                printf ( "! PB6 option chosen\n" ) ;
            break ;
        case 'c' :
        case 'm' :
            if ( bXDL ) {
                fprintf ( stderr, "? conflicting option -%c\n", optch ) ;
                bOptErr = true ;
            } else {
                bMEM = true ;
                if ( optarg != NULL )
                    strcpy ( code_filename, optarg ) ;
            }
            break ;
        case 'd' :
        case 's' :
            if ( bXDL ) {
                fprintf ( stderr, "? conflicting option -%c\n", optch ) ;
                bOptErr = true ;
            } else {
                bMEM = true ;
                if ( optarg != NULL )
                    strcpy ( data_filename, optarg ) ;
            }
            break ;
        case 'h' :
            bOptErr = true ;
            break ;
        case 'p' :
            bPico = true ;
            break ;
        case 'v' :
            bVerbose = true ;
            printf ( "! \'verbose\' option chosen\n" ) ;
            break ;
        case 'x' :
            if ( bMEM ) {
                fprintf ( stderr, "? conflicting option -%c\n", optch ) ;
                bOptErr = true ;
            } else {
                bXDL = true ;
                if ( bVerbose )
                    printf ( "! XDL option chosen\n" ) ;
                if ( optarg != NULL )
                    strcpy ( code_filename, optarg ) ;
            }
            break ;
        default :
            fprintf ( stderr, "? unknown option: -%c\n", optopt ) ;
            bOptErr = true ;
            break ;
        }
    }

    if ( bOptErr ) {
        usage ( basename ( argv[ 0 ] ) ) ;
        exit ( -1 ) ;
    }

    if ( * code_filename != 0 ) {
        if ( strrchr ( code_filename, '.' ) == NULL )
            strcat ( code_filename, bXDL ? ".xdl" : ".mem" ) ;
        if ( bVerbose )
            printf ( "! %s file: %s\n", bXDL ? "XDL" : "MEM", code_filename ) ;
    }

    if ( * data_filename != 0 ) {
        if ( strrchr ( data_filename, '.' ) == NULL )
            strcat ( data_filename, ".scr" ) ;
        if ( bVerbose )
            printf ( "! %s file: %s\n", "SCR", data_filename ) ;
    }

    // output filename
    if ( argv[ optind ] == NULL ) {
        strcpy ( psm_filename, filename ( code_filename ) ) ;
    } else {
        strcpy ( psm_filename, argv[ optind++ ] ) ;
    }
    if ( * psm_filename != 0 ) {
        if ( strrchr ( psm_filename, '.' ) == NULL )
            strcat ( psm_filename, bPico ? ".vhd" : ".psm" ) ;
        if ( bVerbose )
            printf ( "! output file: %s\n", psm_filename ) ;
    }

    for ( i = 0 ; i < MEMSIZE ; i++ )
        Code[ i ] = 0 ;

    if ( bXDL )
        result = loadXDL ( code_filename ) ;
    if ( bMEM )
        result = loadMEM ( code_filename ) && loadMEM ( data_filename ) ;
    if ( ! result )
        exit ( -2 ) ;

    if ( bKCPSM6 ) {
        if ( bPico )
            writeVHD6 ( psm_filename ) ;
        else
            writePSM6 ( psm_filename ) ;
    } else
        writePSM3 ( psm_filename ) ;

    exit ( 0 ) ;
}
