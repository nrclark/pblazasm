/*
 *  Copyright © 2003..2013 : Henk van Kampen <henk@mediatronix.com>
 *
 *  This file is part of pBlazASM.
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

#define MAXMEM 4096
#define MAXSCR 256

typedef struct _instr {
    uint32_t addr ;
    uint32_t code ;
    bool blank ;
    bool breadcrum ;
    bool label ;
} INST_t ;

inline static uint32_t DestReg ( const int code )
{
    return ( code >> 8 ) & 0xF ;
}

inline static uint32_t SrcReg ( const int code )
{
    return ( code >> 4 ) & 0xF ;
}

inline static uint32_t Constant ( const int code )
{
    return code & 0xFF ;
}

inline static uint32_t Address12 ( const int code )
{
    return code & 0xFFF ;
}

bool writeVHD6 ( const char * strPSMfile, INST_t * Code, uint32_t * Data, uint64_t inst_map, int code_size, int stack_size, int pad_size, int bank_size, bool want_alu ) ;
