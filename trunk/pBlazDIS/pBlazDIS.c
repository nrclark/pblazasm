/*
 *  Copyright © 2003..2012 : Henk van Kampen <henk@mediatronix.com>
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

#define MAXMEM 4096
#define MAXSCR 256

typedef struct _instr {
    uint32_t addr ;
    uint32_t code ;
    bool blank ;
    bool breadcrum ;
    bool label ;
} INST_t ;

INST_t Code[ MAXMEM ] ;
uint32_t Data[ MAXSCR ] ;

static void usage ( char * text ) {
    printf ( "\n%s - %s\n", text, "Picoblaze Disassembler utility V2.3" ) ;
    printf ( "\nUSAGE:\n" ) ;
    printf ( "   pBlazDIS [-3|-6]  [-p] [-v] [-x] [-n] -c<MEM code inputfile> -s<MEM data inputfile> <PSM/VHD outputfile>\n" ) ;
    printf ( "   where:\n"
             "         -3      select Picoblaze-3, mandatory\n"
             "         -6      select Picoblaze-6, mandatory\n"
             "         -c      loads a code MEM file\n"
             "         -x      loads an XDL file clip\n"
             "         -n      loads an NDF file clip\n"
             "         -s      loads a data MEM file\n"
             "         -6      use Picoblaze6 opcodes\n"
             "         -v      generates verbose reporting\n"
//             "         -p      generate a PicoCore VHDL file\n"
           ) ;
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
        fprintf ( stderr, "? Unable to open MEM file '%s\n'", strMEMfile ) ;
        return false ;
    }

    for ( addr = -1 ; addr < MAXMEM && fgets ( line, sizeof ( line ), infile ) != NULL; ) {
        if ( ( p = strchr ( line, '@' ) ) != NULL ) {
            if ( sscanf ( ++p, "%X", &addr ) != 1 ) {
                fprintf ( stderr, "? Error in address in MEM file '%s\n'", strMEMfile ) ;
                return false ;
            }
        } else {
            if ( addr == -1 ) {
                fprintf ( stderr, "? Missing address in MEM file '%s', assuming 0\n", strMEMfile ) ;
                addr = 0 ;
                // return false ;
            }
            sscanf ( line, "%X", &code ) ;
            Code[ addr ].addr = addr ;
            Code[ addr ].code = code ;
            Code[ addr ].blank = false ;
            addr += 1 ;
        }
    }

    fclose ( infile ) ;
    return true ;
}

bool loadSCR ( const char * strDatafile, const int offset ) {
    int addr ;
    uint32_t data ;
    char line[ 256 ], *p ;
    FILE * infile = NULL ;

    infile = fopen ( strDatafile, "r" ) ;
    if ( infile == NULL ) {
        fprintf ( stderr, "? Unable to open data SCR file '%s'\n", strDatafile ) ;
        return false ;
    }

    for ( addr = -1 ; fgets ( line, sizeof ( line ), infile ) != NULL; ) {
        if ( ( p = strchr ( line, '@' ) ) != NULL ) {
            if ( sscanf ( ++p, "%X", &addr ) != 1 ) {
                fprintf ( stderr, "? Bad address in data SCR file '%s'\n", strDatafile ) ;
                return false ;
            }
        } else {
            if ( addr == -1 ) {
                fprintf ( stderr, "? Missing address in data SCR file '%s'\n", strDatafile ) ;
                return false ;
            }
            sscanf ( line, "%X", &data ) ;
            Data[ addr & ( MAXSCR - 1 ) ] = data ;
            addr += 1 ;
        }
    }

    fclose ( infile ) ;
    return true ;
}

bool loadNDF ( const char * strNDFfile ) {
    int i, addr, page ;
    uint32_t code ;
    char line[ 256 ], *p, t[ 16 ] ;
    FILE * infile = NULL ;

    if ( strNDFfile == NULL || * strNDFfile == 0 )
        return true ;

    infile = fopen ( strNDFfile, "r" ) ;
    if ( infile == NULL ) {
        fprintf ( stderr, "? Unable to open NDF file '%s'\n", strNDFfile ) ;
        return false ;
    }

    for ( addr = -1 ; addr < MAXMEM + 128 && fgets ( line, sizeof ( line ), infile ) != NULL; ) {
        if ( ( p = strstr ( line, "INITP_" ) ) != NULL ) {
            p += 6 ;
            strncpy ( t, p, 2 ) ;
            t[ 2 ] = 0 ;
            if ( sscanf ( t, "%X", &page ) != 1 ) {
                fprintf ( stderr, "? Error in address in NDF file '%s'\n", strNDFfile ) ;
                return false ;
            } else
                addr = ( page << 7 ) + 127 ;
            p = strstr ( p, "\"" ) ;
            p += 1 ;
// (property INITP_07 (string "0000000000000000000000000000000000000000000000000000000000000000") (owner "Xilinx"))
            for ( i = 0 ; i < 64 ; i += 1 ) {
                strncpy ( t, p, 1 ) ;
                t[ 1 ] = 0 ;
                if ( sscanf ( t, "%X", &code ) != 1 ) {
                    fprintf ( stderr, "? Error in parity data in NDF file '%s'\n", strNDFfile ) ;
                    return false ;
                }
                Code[ addr ].addr = addr;
                Code[ addr-- ].code |= ( code << 14 ) & 0x30000 ;
                Code[ addr ].addr = addr;
                Code[ addr-- ].code |= ( code << 16 ) & 0x30000 ;
                p += 1 ;
            }
        } else if ( ( p = strstr ( line, "INIT_" ) ) != NULL ) {
            p += 5 ;
            strncpy ( t, p, 2 ) ;
            t[ 2 ] = 0 ;
            if ( sscanf ( t, "%X", &page ) != 1 ) {
                fprintf ( stderr, "? Error in address in NDF file '%s'\n", strNDFfile ) ;
                return false ;
            } else
                addr = ( page << 4 ) + 15 ;
            p = strstr ( p, "\"" ) ;
            p += 1 ;
// (property INIT_3F (string "0000000000000000000000000000000000000000000000000000000000000000") (owner "Xilinx"))
            for ( i = 0 ; i < 16 ; i += 1 ) {
                strncpy ( t, p, 4 ) ;
                t[ 4 ] = 0 ;
                if ( sscanf ( t, "%X", &code ) != 1 ) {
                    fprintf ( stderr, "? Error in data in NDF file '%s'\n", strNDFfile ) ;
                    return false ;
                }
                Code[ addr ].addr = addr;
                Code[ addr-- ].code |= code ;
                p += 4 ;
            }
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
        fprintf ( stderr, "? Unable to open XDL file '%s'\n", strXDLfile ) ;
        return false ;
    }

    for ( addr = -1 ; addr < MAXMEM + 128 && fgets ( line, sizeof ( line ), infile ) != NULL; ) {
        if ( ( p = strstr ( line, "INITP_" ) ) != NULL ) {
            p += 6 ;
            strncpy ( t, p, 2 ) ;
            t[ 2 ] = 0 ;
            if ( sscanf ( t, "%X", &page ) != 1 ) {
                fprintf ( stderr, "? Error in address in XDL file '%s'\n", strXDLfile ) ;
                return false ;
            } else
                addr = ( page << 7 ) + 127 ;
            p += 4 ;
            for ( i = 0 ; i < 64 ; i += 1 ) {
                strncpy ( t, p, 1 ) ;
                t[ 1 ] = 0 ;
                if ( sscanf ( t, "%X", &code ) != 1 ) {
                    fprintf ( stderr, "? Error in parity data in XDL file '%s'\n", strXDLfile ) ;
                    return false ;
                }
                Code[ addr ].addr = addr;
                Code[ addr-- ].code |= ( code << 14 ) & 0x30000 ;
                Code[ addr ].addr = addr;
                Code[ addr-- ].code |= ( code << 16 ) & 0x30000 ;
                p += 1 ;
            }
        } else if ( ( p = strstr ( line, "INIT_" ) ) != NULL ) {
            p += 5 ;
            strncpy ( t, p, 2 ) ;
            t[ 2 ] = 0 ;
            if ( sscanf ( t, "%X", &page ) != 1 ) {
                fprintf ( stderr, "? Error in address in XDL file '%s'\n", strXDLfile ) ;
                return false ;
            } else
                addr = ( page << 4 ) + 15 ;
            p += 4 ;
            for ( i = 0 ; i < 16 ; i += 1 ) {
                strncpy ( t, p, 4 ) ;
                t[ 4 ] = 0 ;
                if ( sscanf ( t, "%X", &code ) != 1 ) {
                    fprintf ( stderr, "? Error in data in XDL file '%s'\n", strXDLfile ) ;
                    return false ;
                }
                Code[ addr ].addr = addr;
                Code[ addr-- ].code |= code ;
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

static char * Condition3 ( const int code ) {
    char * Conditions[ 4 ] = { "Z", "NZ", "C", "NC" } ;
    return  Conditions[ ( code >> 10 ) & 0x3 ] ;
}

static void fprintline0 ( FILE * file, char * opcode, int pc, int c ) {
    int n = 0 ;

    n += fprintf ( file, "%*s", 16, "" ) ;
    n += fprintf ( file, "%*s", -6, opcode ) ;
    n += fprintf ( file, "%*s", 40 - n, "" ) ;
    n += fprintf ( file, "; %03X : %05X \n", pc, c ) ;
}
static void fprintline1 ( FILE * file, char * opcode, char * f1, int p1, int pc, int c ) {
    int n = 0 ;

    n += fprintf ( file, "%*s", 16, "" ) ;
    n += fprintf ( file, "%*s", -6, opcode ) ;
    n += fprintf ( file, f1, p1 ) ;
    n += fprintf ( file, "%*s", 40 - n, "" ) ;
    n += fprintf ( file, "; %03X : %05X \n", pc, c ) ;
}
static void fprintline2 ( FILE * file, char * opcode, char * f1, char * f2, int p1, int p2, int pc, int c ) {
    int n = 0 ;

    n += fprintf ( file, "%*s", 16, "" ) ;
    n += fprintf ( file, "%*s", -6, opcode ) ;
    n += fprintf ( file, f1, p1 ) ;
    n += fprintf ( file, ", " ) ;
    n += fprintf ( file, f2 , p2 ) ;
    n += fprintf ( file, "%*s", 40 - n, "" ) ;
    n += fprintf ( file, "; %03X : %05X \n", pc, c ) ;
}
static void fprintline3 ( FILE * file, char * opcode, char * p1, char * f2, int p2, int pc, int c ) {
    int n = 0 ;

    n += fprintf ( file, "%*s", 16, "" ) ;
    n += fprintf ( file, "%*s", -6, opcode ) ;
    n += fprintf ( file, p1 ) ;
    n += fprintf ( file, ", " ) ;
    n += fprintf ( file, f2 , p2 ) ;
    n += fprintf ( file, "%*s", 40 - n, "" ) ;
    n += fprintf ( file, "; %03X : %05X \n", pc, c ) ;
}

static void fprintline4 ( FILE * file, char * opcode, char * p1, int pc, int c ) {
    int n = 0 ;

    n += fprintf ( file, "%*s", 16, "" ) ;
    n += fprintf ( file, "%*s", -6, opcode ) ;
    n += fprintf ( file, p1 ) ;
    n += fprintf ( file, "%*s", 40 - n, "" ) ;
    n += fprintf ( file, "; %03X : %05X \n", pc, c ) ;
}

static bool writePSM3 ( const char * strPSMfile ) {
    FILE * outfile = NULL ;
    int pc = 0 ;
    int c = 0 ;
    enum {
        stIDLE, stCODE, stDATA
    } state = stIDLE ;

    outfile = fopen ( strPSMfile, "w" ) ;
    if ( outfile == NULL ) {
        fprintf ( stderr, "? Unable to open output file '%s'\n", strPSMfile ) ;
        return false ;
    }
    for ( pc = 0 ; pc < 1024 ; ) {
        c = Code[ pc ].code & 0x3FFFF ;
        switch ( state ) {
        case stIDLE :
            if ( c != 0 ) {
                switch ( pc ) {
                case 0x380 :
                    fprintf ( outfile, "\n\t.SCR\t0x%03X \n", pc ) ;
                    state = stDATA ;
                    break ;
                default :
                    fprintf ( outfile, "\n\t.ORG\t0x%03X \n", pc ) ;
                    state = stCODE ;
                }
            } else
                pc += 1 ;
            break ;
        case stCODE :
            if ( Code[ pc ].label )
                fprintf ( outfile, "\nL_%03X : \n", pc ) ;
            if ( !Code[ pc ].breadcrum )
                fprintf ( outfile, "// not reached:\n" ) ;

            if ( !Code[ pc ].blank ) {
                switch ( c ) {
                case 0x00000 ... 0x00FFF :
                    if ( Constant ( c ) < 10 )
                        fprintline2 ( outfile, "MOVE", "s%X", "%d", DestReg ( c ), Constant ( c ), pc, c ) ;
                    else
                        fprintline2 ( outfile, "MOVE", "s%X", "0x%02X", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x01000 ... 0x01FFF :
                    fprintline2 ( outfile, "MOVE", "s%X", "s%X", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x0A000 ... 0x0AFFF :
                    fprintline2 ( outfile, "AND", "s%X", "0x%02X", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x0B000 ... 0x0BFFF :
                    fprintline2 ( outfile, "AND", "s%X", "s%X", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x0C000 ... 0x0CFFF :
                    fprintline2 ( outfile, "OR", "s%X", "0x%02X", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x0D000 ... 0x0DFFF :
                    fprintline2 ( outfile, "OR", "s%X", "s%X", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x0E000 ... 0x0EFFF :
                    fprintline2 ( outfile, "XOR", "s%X", "0x%02X", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x0F000 ... 0x0FFFF :
                    fprintline2 ( outfile, "XOR", "%X", "s%X", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x12000 ... 0x12FFF :
                    fprintline2 ( outfile, "TEST", "s%X", "0x%02X", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x13000 ... 0x13FFF :
                    fprintline2 ( outfile, "TEST", "%X", "s%X", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x18000 ... 0x18FFF :
                    fprintline2 ( outfile, "ADD", "s%X", "0x%02X", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x19000 ... 0x19FFF :
                    fprintline2 ( outfile, "ADD", "%X", "s%X", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x1A000 ... 0x1AFFF :
                    fprintline2 ( outfile, "ADDC", "s%X", "0x%02X", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x1B000 ... 0x1BFFF :
                    fprintline2 ( outfile, "ADDC", "%X", "s%X", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x1C000 ... 0x1CFFF :
                    fprintline2 ( outfile, "SUB", "s%X", "0x%02X", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x1D000 ... 0x1DFFF :
                    fprintline2 ( outfile, "SUB", "%X", "s%X", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x1E000 ... 0x1EFFF :
                    fprintline2 ( outfile, "SUBC", "s%X", "0x%02X", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x1F000 ... 0x1FFFF :
                    fprintline2 ( outfile, "SUBC", "%X", "s%X", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x14000 ... 0x14FFF :
                    fprintline2 ( outfile, "COMP", "s%X", "0x%02X", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x15000 ... 0x15FFF :
                    fprintline2 ( outfile, "COMP", "%X", "s%X", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x20000 ... 0x20FFF :
                    switch ( c & 0xF ) {
                    case 0x2 :
                        fprintline1 ( outfile, "RL", "s%X", DestReg ( c ), pc, c ) ;
                        break ;
                    case 0x6 :
                        fprintline1 ( outfile, "SL0", "s%X", DestReg ( c ), pc, c ) ;
                        break ;
                    case 0x7 :
                        fprintline1 ( outfile, "SL1", "s%X", DestReg ( c ), pc, c ) ;
                        break ;
                    case 0x0 :
                        fprintline1 ( outfile, "SLA", "s%X", DestReg ( c ), pc, c ) ;
                        break ;
                    case 0x4 :
                        fprintline1 ( outfile, "SLX", "s%X", DestReg ( c ), pc, c ) ;
                        break ;

                    case 0xC :
                        fprintline1 ( outfile, "RR", "s%X", DestReg ( c ), pc, c ) ;
                        break ;
                    case 0xE :
                        fprintline1 ( outfile, "SR0", "s%X", DestReg ( c ), pc, c ) ;
                        break ;
                    case 0xF :
                        fprintline1 ( outfile, "SR1", "s%X", DestReg ( c ), pc, c ) ;
                        break ;
                    case 0x8 :
                        fprintline1 ( outfile, "SRA", "s%X", DestReg ( c ), pc, c ) ;
                        break ;
                    case 0xA :
                        fprintline1 ( outfile, "SRX", "s%X", DestReg ( c ), pc, c ) ;
                        break ;

                    default :
                        fprintline1 ( outfile, "INST", "0x%05X", c, pc, c ) ;
                    }
                    break ;

                case 0x34000 ... 0x34FFF :
                    fprintline1 ( outfile, "JUMP", "L_%03X", Address10 ( c ), pc, c ) ;
                    break ;
                case 0x35000 ... 0x35FFF :
                    fprintline3 ( outfile, "JUMP", Condition3 ( c ), "L_%03X", Address10 ( c ), pc, c ) ;
                    break ;

                case 0x30000 ... 0x30FFF :
                    fprintline1 ( outfile, "CALL", "L_%03X", Address10 ( c ), pc, c ) ;
                    break ;
                case 0x31000 ... 0x31FFF :
                    fprintline3 ( outfile, "CALL", Condition3 ( c ), "L_%03X", Address10 ( c ), pc, c ) ;
                    break ;

                case 0x2A000 ... 0x2AFFF :
                    fprintline0 ( outfile, "RET", pc, c ) ;
                    break ;
                case 0x2B000 ... 0x2BFFF :
                    fprintline4 ( outfile, "RET", Condition3 ( c ), pc, c ) ;
                    break ;

                case 0x2E000 ... 0x2EFFF :
                    fprintline2 ( outfile, "ST", "s%X", "0x%02X", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x2F000 ... 0x2FFFF :
                    fprintline2 ( outfile, "ST", "s%X", "s%X", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x06000 ... 0x06FFF :
                    fprintline2 ( outfile, "LD", "s%X", "0x%02X", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x07000 ... 0x07FFF :
                    fprintline2 ( outfile, "LD", "s%X", "s%X", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x2C000 ... 0x2CFFF :
                    fprintline2 ( outfile, "OUT", "s%X", "0x%02X", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x2D000 ... 0x2DFFF :
                    fprintline2 ( outfile, "OUT", "s%X", "s%X", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x04000 ... 0x04FFF :
                    fprintline2 ( outfile, "IN", "s%X", "0x%02X", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x05000 ... 0x05FFF :
                    fprintline2 ( outfile, "IN", "s%X", "s%X", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x3C000 :
                    fprintline0 ( outfile, "DINT", pc, c ) ;
                    break ;
                case 0x3C001 :
                    fprintline0 ( outfile, "EINT", pc, c ) ;
                    break ;
                case 0x38000 :
                    fprintline4 ( outfile, "RETI", "DISABLE", pc, c ) ;
                    break ;
                case 0x38001 :
                    fprintline4 ( outfile, "RETI", "ENABLE", pc, c ) ;
                    break ;

                default :
                    fprintline1 ( outfile, "INST", "0x%05X", c, pc, c ) ;
                }
                pc += 1 ;
            } else
                state = stIDLE ;
            break ;
        case stDATA :
            if ( c != 0 ) {
                fprintf ( outfile, "\t.BYT\t0x%02X, 0x%02X\t; %03X : %05X \n", c & 0xFF, ( c >> 8 ) & 0xFF, pc, c ) ;
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
    for ( pc = 0 ; pc < MAXMEM ; ) {
        c = Code[ pc ].code & 0x3FFFF ;
        switch ( state ) {
        case stIDLE :
            if ( c != 0 ) {
                switch ( pc ) {
                case 0x380 :
                    fprintf ( outfile, "\n\t\t.SCR\t0x%03X \n", pc ) ;
                    state = stDATA ;
                    break ;
                default :
                    fprintf ( outfile, "\n\t\t.ORG\t0x%03X \n", pc ) ;
                    state = stCODE ;
                }
            } else
                pc += 1 ;
            break ;
        case stCODE :
            if ( Code[ pc ].label )
                fprintf ( outfile, "\nL_%03X : \n", pc ) ;
            if ( !Code[ pc ].breadcrum )
                fprintf ( outfile, "// not reached:\n" ) ;

            if ( !Code[ pc ].blank ) {
                switch ( c ) {
                case 0x00000 ... 0x00FFF :
                    fprintline2 ( outfile, "MOVE", "s%X", "s%X", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x01000 ... 0x01FFF :
                    if ( Constant ( c ) < 10 )
                        fprintline2 ( outfile, "MOVE", "s%X", "%d", DestReg ( c ), Constant ( c ), pc, c ) ;
                    else
                        fprintline2 ( outfile, "MOVE", "s%X", "0x%02X", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x16000 ... 0x16FFF :
                    fprintf ( outfile, "\t\tSTAR\ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x02000 ... 0x02FFF :
                    fprintf ( outfile, "\t\tAND \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x03000 ... 0x03FFF :
                    fprintf ( outfile, "\t\tAND \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x04000 ... 0x04FFF :
                    fprintf ( outfile, "\t\tOR  \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x05000 ... 0x05FFF :
                    fprintf ( outfile, "\t\tOR  \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x06000 ... 0x06FFF :
                    fprintf ( outfile, "\t\tXOR \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x07000 ... 0x07FFF :
                    fprintf ( outfile, "\t\tXOR \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x0C000 ... 0x0CFFF :
                    fprintf ( outfile, "\t\tTEST\ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x0D000 ... 0x0DFFF :
                    fprintf ( outfile, "\t\tTEST\ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x0E000 ... 0x0EFFF :
                    fprintf ( outfile, "\t\tTSTC\ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x0F000 ... 0x0FFFF :
                    fprintf ( outfile, "\t\tTSTC\ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x10000 ... 0x10FFF :
                    fprintf ( outfile, "\t\tADD \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x11000 ... 0x11FFF :
                    fprintf ( outfile, "\t\tADD \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x12000 ... 0x12FFF :
                    fprintf ( outfile, "\t\tADDC\ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x13000 ... 0x13FFF :
                    fprintf ( outfile, "\t\tADDC\ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x18000 ... 0x18FFF :
                    fprintf ( outfile, "\t\tSUB \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x19000 ... 0x19FFF :
                    fprintf ( outfile, "\t\tSUB \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x1A000 ... 0x1AFFF :
                    fprintf ( outfile, "\t\tSUBC\ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x1B000 ... 0x1BFFF :
                    fprintf ( outfile, "\t\tSUBC\ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x1C000 ... 0x1CFFF :
                    fprintf ( outfile, "\t\tCOMP\ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x1D000 ... 0x1DFFF :
                    fprintf ( outfile, "\t\tCOMP\ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x1E000 ... 0x1EFFF :
                    fprintf ( outfile, "\t\tCMPC\ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x1F000 ... 0x1FFFF :
                    fprintf ( outfile, "\t\tCMPC\ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x14000 ... 0x14FFF :
                    if ( c & 0xF0 ) {
                        fprintf ( outfile, "\t\tCORE\ts%X   \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                    } else
                        switch ( c & 0xF ) {
                        case 0x2 :
                            fprintf ( outfile, "\t\tRL  \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                            break ;
                        case 0x6 :
                            fprintf ( outfile, "\t\tSL0 \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                            break ;
                        case 0x7 :
                            fprintf ( outfile, "\t\tSL1 \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                            break ;
                        case 0x0 :
                            fprintf ( outfile, "\t\tSLA \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                            break ;
                        case 0x4 :
                            fprintf ( outfile, "\t\tSLX \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                            break ;

                        case 0xC :
                            fprintf ( outfile, "\t\tRR  \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                            break ;
                        case 0xE :
                            fprintf ( outfile, "\t\tSR0 \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                            break ;
                        case 0xF :
                            fprintf ( outfile, "\t\tSR1 \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                            break ;
                        case 0x8 :
                            fprintf ( outfile, "\t\tSRA \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                            break ;
                        case 0xA :
                            fprintf ( outfile, "\t\tSRX \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                            break ;

                        default :
                            fprintf ( outfile, "\t\tINST\t0x%05X\t; %03X : %05X \n", c, pc, c ) ;
                        }
                    break ;

                case 0x22000 ... 0x22FFF :
                    fprintf ( outfile, "\t\tJUMP\tL_%03X      \t; %03X : %05X \n\n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x32000 ... 0x32FFF :
                    fprintf ( outfile, "\t\tJUMP\tZ, L_%03X\t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x36000 ... 0x36FFF :
                    fprintf ( outfile, "\t\tJUMP\tNZ, L_%03X\t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x3A000 ... 0x3AFFF :
                    fprintf ( outfile, "\t\tJUMP\tC, 0x%03X\t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x3E000 ... 0x3EFFF :
                    fprintf ( outfile, "\t\tJUMP\tNC, L_%03X\t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x26000 ... 0x26FFF :
                    fprintf ( outfile, "\t\tJUMP\ts%X, s%X  \t; %03X : %05X \n\n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x20000 ... 0x20FFF :
                    fprintf ( outfile, "\t\tCALL\tL_%03X      \t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x30000 ... 0x30FFF :
                    fprintf ( outfile, "\t\tCALL\tZ, L_%03X\t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x34000 ... 0x34FFF :
                    fprintf ( outfile, "\t\tCALL\tNZ, L_%03X\t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x38000 ... 0x38FFF :
                    fprintf ( outfile, "\t\tCALL\tC, L_%03X\t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x3C000 ... 0x3CFFF :
                    fprintf ( outfile, "\t\tCALL\tNC, L_%03X\t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
                    break ;
                case 0x24000 ... 0x24FFF :
                    fprintf ( outfile, "\t\tCALL\ts%X, s%X  \t; %03X : %05X  \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;

                case 0x25000 ... 0x25FFF :
                    fprintf ( outfile, "\t\tRET \t         \t; %03X : %05X \n\n", pc, c ) ;
                    break ;
                case 0x31000 ... 0x31FFF :
                    fprintf ( outfile, "\t\tRET \t %s       \t; %03X : %05X \n", "Z", pc, c ) ;
                    break ;
                case 0x35000 ... 0x35FFF :
                    fprintf ( outfile, "\t\tRET \t %s       \t; %03X : %05X \n", "NZ", pc, c ) ;
                    break ;
                case 0x39000 ... 0x39FFF :
                    fprintf ( outfile, "\t\tRET \t %s       \t; %03X : %05X \n", "C", pc, c ) ;
                    break ;
                case 0x3D000 ... 0x3DFFF :
                    fprintf ( outfile, "\t\tRET \t %s       \t; %03X : %05X \n", "NC", pc, c ) ;
                    break ;
                case 0x21000 ... 0x21FFF :
                    if ( Constant ( c ) < 10 )
                        fprintf ( outfile, "\t\tRET \ts%X, %d   \t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    else
                        fprintf ( outfile, "\t\tRET \ts%X, 0x%02X\t; %03X : %05X \n\n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;


                case 0x29000 :
                    fprintf ( outfile, "\t\tRETI\tDISABLE\t; %03X : %05X \n", pc, c ) ;
                    break ;
                case 0x29001 :
                    fprintf ( outfile, "\t\tRETI\tENABLE\t; %03X : %05X \n", pc, c ) ;
                    break ;

                case 0x2E000 ... 0x2EFFF :
                    fprintf ( outfile, "\t\tST  \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x2F000 ... 0x2FFFF :
                    fprintf ( outfile, "\t\tST  \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x0A000 ... 0x0AFFF :
                    fprintf ( outfile, "\t\tLD  \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x0B000 ... 0x0BFFF :
                    fprintf ( outfile, "\t\tLD  \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x2C000 ... 0x2CFFF :
                    fprintf ( outfile, "\t\tOUT \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x2D000 ... 0x2DFFF :
                    fprintf ( outfile, "\t\tOUT \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;
                case 0x2B000 ... 0x2BFFF :
                    fprintf ( outfile, "\t\tOUTK\t0x%02X, 0x%X\t; %03X : %05X \n", ( c >> 4 ) & 0xFF, c & 0xF, pc, c ) ;
                    break ;

                case 0x08000 ... 0x08FFF :
                    fprintf ( outfile, "\t\tIN  \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                    break ;
                case 0x09000 ... 0x09FFF :
                    fprintf ( outfile, "\t\tIN  \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
                    break ;

                case 0x28000 :
                    fprintf ( outfile, "\t\tDINT\t \t; %03X : %05X \n", pc, c ) ;
                    break ;
                case 0x28001 :
                    fprintf ( outfile, "\t\tEINT\t \t; %03X : %05X \n", pc, c ) ;
                    break ;
                case 0x37000 :
                    fprintf ( outfile, "\t\tBANK\tA\t; %03X : %05X \n", pc, c ) ;
                    break ;
                case 0x37001 :
                    fprintf ( outfile, "\t\tBANK\tB\t; %03X : %05X \n", pc, c ) ;
                    break ;

                default :
                    fprintf ( outfile, "\t\tINST\t0x%05X\t; %03X : %05X \n", pc, c, c ) ;
                }
                pc += 1 ;
            } else
                state = stIDLE ;
            break ;
        case stDATA :
            if ( c != 0 ) {
                fprintf ( outfile, "\t\t.BYT\t0x%02X, 0x%02X\t; %03X : %05X \n", c & 0xFF, ( c >> 8 ) & 0xFF, pc, c ) ;
                pc += 1 ;
            } else
                state = stIDLE ;
            break ;
        }
    }
    fclose ( outfile ) ;
    return true ;
}


void checkPB3 ( int pc ) {
    uint32_t c ;

    Code[ pc ].label = true ;

    while ( pc < 1024 ) {
        if ( Code[ pc ].breadcrum )
            return ;

        c = Code[ pc ].code & 0x3FFFF ;

        switch ( c ) {
// OPs
        case 0x00000 ... 0x00FFF :
        case 0x01000 ... 0x01FFF :
        case 0x0A000 ... 0x0AFFF :
        case 0x0B000 ... 0x0BFFF :
        case 0x0C000 ... 0x0CFFF :
        case 0x0D000 ... 0x0DFFF :
        case 0x0E000 ... 0x0EFFF :
        case 0x0F000 ... 0x0FFFF :
        case 0x12000 ... 0x12FFF :
        case 0x13000 ... 0x13FFF :
        case 0x18000 ... 0x18FFF :
        case 0x19000 ... 0x19FFF :
        case 0x1A000 ... 0x1AFFF :
        case 0x1B000 ... 0x1BFFF :
        case 0x1C000 ... 0x1CFFF :
        case 0x1D000 ... 0x1DFFF :
        case 0x1E000 ... 0x1EFFF :
        case 0x1F000 ... 0x1FFFF :
        case 0x14000 ... 0x14FFF :
        case 0x15000 ... 0x15FFF :
        case 0x2E000 ... 0x2EFFF :
        case 0x2F000 ... 0x2FFFF :
        case 0x06000 ... 0x06FFF :
        case 0x07000 ... 0x07FFF :
        case 0x2C000 ... 0x2CFFF :
        case 0x2D000 ... 0x2DFFF :
        case 0x04000 ... 0x04FFF :
        case 0x05000 ... 0x05FFF :

        case 0x3C000 ... 0x3C001 :
            Code[ pc ].breadcrum = true ;
            pc = pc + 1 ;
            break ;

// SHIFT
        case 0x20000 ... 0x20FFF :
            switch ( c & 0xF ) {
            case 0x2 :
            case 0x6 :
            case 0x7 :
            case 0x0 :
            case 0x4 :

            case 0xC :
            case 0xE :
            case 0xF :
            case 0x8 :
            case 0xA :
                Code[ pc ].breadcrum = true ;
                pc = pc + 1 ;
                break ;
            }
            break ;

// JUMP
        case 0x34000 ... 0x34FFF :
            Code[ pc ].breadcrum = true ;
            checkPB3 ( Address10 ( c ) ) ;
            break ;

// JUMP cond, CALL, CALL cond
        case 0x35000 ... 0x35FFF :
        case 0x30000 ... 0x30FFF :
        case 0x31000 ... 0x31FFF :
            checkPB3 ( Address10 ( c ) ) ;
            Code[ pc ].breadcrum = true ;
            pc = pc + 1 ;
            break ;

// RET
        case 0x2A000 ... 0x2AFFF :
// RETI
        case 0x38000 ... 0x38001 :
            Code[ pc ].breadcrum = true ;
            return ;

// RET cond
        case 0x2B000 ... 0x2BFFF :
            Code[ pc ].breadcrum = true ;
            pc = pc + 1 ;
            break ;

        default :
            return ;
        }
    }
}

void checkPB6 ( int pc ) {
    uint32_t c ;

    Code[ pc ].label = true ;
    while ( pc < 1024 ) {
        if ( Code[ pc ].breadcrum )
            return ;

        c = Code[ pc ].code & 0x3FFFF ;

        switch ( c ) {
// OPs
        case 0x00000 ... 0x00FFF :
        case 0x01000 ... 0x01FFF :
        case 0x16000 ... 0x16FFF :
        case 0x02000 ... 0x02FFF :
        case 0x03000 ... 0x03FFF :
        case 0x04000 ... 0x04FFF :
        case 0x05000 ... 0x05FFF :
        case 0x06000 ... 0x06FFF :
        case 0x07000 ... 0x07FFF :
        case 0x0C000 ... 0x0CFFF :
        case 0x0D000 ... 0x0DFFF :
        case 0x0E000 ... 0x0EFFF :
        case 0x0F000 ... 0x0FFFF :
        case 0x10000 ... 0x10FFF :
        case 0x11000 ... 0x11FFF :
        case 0x12000 ... 0x12FFF :
        case 0x13000 ... 0x13FFF :
        case 0x18000 ... 0x18FFF :
        case 0x19000 ... 0x19FFF :
        case 0x1A000 ... 0x1AFFF :
        case 0x1B000 ... 0x1BFFF :
        case 0x1C000 ... 0x1CFFF :
        case 0x1D000 ... 0x1DFFF :
        case 0x1E000 ... 0x1EFFF :
        case 0x1F000 ... 0x1FFFF :

        case 0x2E000 ... 0x2EFFF :
        case 0x2F000 ... 0x2FFFF :
        case 0x0A000 ... 0x0AFFF :
        case 0x0B000 ... 0x0BFFF :
        case 0x2C000 ... 0x2CFFF :
        case 0x2D000 ... 0x2DFFF :
        case 0x2B000 ... 0x2BFFF :
        case 0x08000 ... 0x08FFF :
        case 0x09000 ... 0x09FFF :

        case 0x28000 ... 0x28001 :
        case 0x37000 ... 0x37001 :
            Code[ pc ].breadcrum = true ;
            pc = pc + 1 ;
            break ;

// RR, SR, RL, SL
        case 0x14000 ... 0x14FFF :
            switch ( c & 0xF ) {
            case 0x2 :
            case 0x6 :
            case 0x7 :
            case 0x0 :
            case 0x4 :

            case 0xC :
            case 0xE :
            case 0xF :
            case 0x8 :
            case 0xA :
                Code[ pc ].breadcrum = true ;
                pc = pc + 1 ;
                break ;
            }
            break ;

// JUMP
        case 0x22000 ... 0x22FFF :
            Code[ pc ].breadcrum = true ;
            checkPB6 ( Address12 ( c ) ) ;
            break ;

// JUMP cond , CALL, CALL cond
        case 0x32000 ... 0x32FFF :
        case 0x36000 ... 0x36FFF :
        case 0x3A000 ... 0x3AFFF :
        case 0x3E000 ... 0x3EFFF :
        case 0x26000 ... 0x26FFF :

        case 0x20000 ... 0x20FFF :
        case 0x30000 ... 0x30FFF :
        case 0x34000 ... 0x34FFF :
        case 0x38000 ... 0x38FFF :
        case 0x3C000 ... 0x3CFFF :
        case 0x24000 ... 0x24FFF :
            checkPB6 ( Address12 ( c ) ) ;
            Code[ pc ].breadcrum = true ;
            pc = pc + 1 ;
            break ;

// RET, RETI
        case 0x25000 ... 0x25FFF :
        case 0x29000 ... 0x29001 :
            Code[ pc ].breadcrum = true ;
            return ;

// RET cond
        case 0x31000 ... 0x31FFF :
        case 0x35000 ... 0x35FFF :
        case 0x39000 ... 0x39FFF :
        case 0x3D000 ... 0x3DFFF :
        case 0x21000 ... 0x21FFF :
            Code[ pc ].breadcrum = true ;
            pc = pc + 1 ;
            break ;

        default :
            return ;
        }
    }
}

void checkCode ( bool bCore ) {
    if ( bCore )
        checkPB6 ( 0 ) ;
    else
        checkPB3 ( 0 ) ;
}

int main ( int argc, char * argv[] ) {
    char code_filename[ 256 ] = { '\0' } ;
    char data_filename[ 256 ] = { '\0' } ;
    char psm_filename[ 256 ] = { '\0' } ;

    bool result = false ;
    bool bOptErr = false ;
    bool bKCPSM6 = true ;
    bool bVerbose = false ;
    bool bMEM = false ;
    bool bSCR = false ;
    bool bXDL = false ;
    bool bNDF = false ;
    bool bPico = false ;

    extern char * optarg ;
    extern int optind, optopt, opterr ;
    int i, optch ;

    opterr = -1 ;
    while ( ( optch = getopt ( argc, argv, "36c:d:hm:n:ps:x:v" ) ) != -1 ) {
        switch ( optch ) {
        case '3' :
            bKCPSM6 = false ;
            if ( bVerbose )
                printf ( "! PB3 option chosen\n" ) ;
            break ;
        case '6' :
            bKCPSM6 = true ;
            if ( bVerbose )
                printf ( "! PB6 option chosen\n" ) ;
            break ;
        case 'c' :
        case 'm' :
            if ( bXDL | bNDF ) {
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
            if ( bXDL | bNDF ) {
                fprintf ( stderr, "? conflicting option -%c\n", optch ) ;
                bOptErr = true ;
            } else {
                bSCR = true ;
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
        case 'n' :
            if ( bMEM | bSCR | bXDL ) {
                fprintf ( stderr, "? conflicting option -%c\n", optch ) ;
                bOptErr = true ;
            } else {
                bNDF = true ;
                if ( bVerbose )
                    printf ( "! NDF option chosen\n" ) ;
                if ( optarg != NULL )
                    strcpy ( code_filename, optarg ) ;
            }
            break ;
        case 'x' :
            if ( bMEM | bSCR | bNDF ) {
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
            strcat ( code_filename, bXDL ? ".xdl" : bNDF ? ".ndf" : ".mem" ) ;
        if ( bVerbose )
            printf ( "! %s file: %s\n", bXDL ? "XDL" : bNDF ? "NDF" : "MEM", code_filename ) ;
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

    for ( i = 0 ; i < MAXMEM ; i++ ) {
        Code[ i ].addr = i ;
        Code[ i ].code = 0 ;
        Code[ i ].blank = true ;
        Code[ i ].breadcrum = false ;
        Code[ i ].label = false ;
    }

    for ( i = 0 ; i < MAXSCR ; i++ )
        Data[ i ] = 0 ;

    if ( bXDL )
        result = loadXDL ( code_filename ) ;
    if ( bNDF )
        result = loadNDF ( code_filename ) ;
    if ( bMEM )
        result = loadMEM ( code_filename ) ;
    if ( bSCR )
        result &= loadSCR ( data_filename, 0 ) ;
    if ( ! result )
        exit ( -2 ) ;

    checkCode ( bKCPSM6 ) ;

    if ( bKCPSM6 ) {
//        if ( bPico )
//            writeVHD6 ( psm_filename ) ;
//        else
        writePSM6 ( psm_filename ) ;
    } else
        writePSM3 ( psm_filename ) ;

    exit ( 0 ) ;
}
