/*
 *  Copyright © 2003..2012 : Henk van Kampen <henk@mediatronix.com>
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

#include <stddef.h>
#include <stdint.h>

#include "pBlaze.h"

Picoblaze::Picoblaze( void ) {
    bPB3 = false ;

    clearCode() ;
    bank = 0 ;

    for ( int io = 0 ; io < MAXIO ; io += 1 ) {
        IO[ io ].device = NULL ;
        IO[ io ].item = NULL ;
    }

    for ( int scr = 0 ; scr < MAXSCR ; scr += 1 ) {
        scratchpad[ scr ].value = 0 ;
        scratchpad[ scr ].item = NULL ;
    }
}

Picoblaze::~Picoblaze( void ) {
    for ( int io = 0 ; io < MAXIO ; io += 1 )
        if ( IO[ io ].device != NULL )
            IO[ io ].device->IODevice::~IODevice() ;
}

void Picoblaze::clearCode() {
    for ( int address = 0 ; address < MAXMEM ; address += 1 ) {
        Code[ address ].code = 0 ;
        Code[ address ].count = 0ll ;
        Code[ address ].line = 0 ;
        Code[ address ].breakpoint = true ;
        Code[ address ].item = NULL ;
    }
}

void Picoblaze::clearScratchpad( void ) {
    for ( int scr = 0 ; scr < MAXSCR ; scr += 1 )
        scratchpad[ scr ].value = 0 ;
}

void Picoblaze::initPB6 ( void ) {
    pc = 0 ;
    sp = 0 ;
    zero = false ;
    carry = false ;
    enable = false ;
    bank = 0 ;

    for ( int reg = 0 ; reg < MAXREG ; reg += 1 ) {
        registers[ 0 ][ reg ].value = 0 ;
        registers[ 0 ][ reg ].defined = false ;
        registers[ 1 ][ reg ].value = 0 ;
        registers[ 1 ][ reg ].defined = false ;
    }

    for ( int sp = 0 ; sp < MAXSTK ; sp += 1 ) {
        stack[ sp ].pc = 0 ;
        stack[ sp ].carry = false ;
        stack[ sp ].zero = false ;
    }

    for ( int address = 0 ; address < MAXMEM ; address += 1 )
        Code[ address ].count = 0ll ;

    CycleCounter = 0ll ;
}

void Picoblaze::resetPB6 ( void ) {
    pc = 0 ;
    sp = 0 ;
    zero = false ;
    carry = false ;
    enable = false ;

    for ( int address = 0 ; address < MAXMEM ; address += 1 )
        Code[ address ].count = 0ll ;

    CycleCounter = 0ll ;
}

bool Picoblaze::stepPB6 ( void ) {
    uint32_t c, t ;
    int addr = 0 ;

    c = Code[ pc ].code & 0x3FFFF ;
    if ( bPB3 ) // PB3 flag
        c |= 0x100000 ; // specifies a different opcode set
    npc = pc + 1 ;

    switch ( c ) {
    case 0x00000 ... 0x00FFF : // MOVE sX, sY
    case 0x101000 ... 0x101FFF :
        registers[ bank ][ DestReg ( c ) ].value = registers[ bank ][ SrcReg ( c ) ].value ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        break ;
    case 0x01000 ... 0x01FFF : // MOVE sX, K
    case 0x100000 ... 0x100FFF :
        registers[ bank ][ DestReg ( c ) ].value = Constant ( c ) ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        break ;
    case 0x16000 ... 0x16FFF : // STAR
        registers[ bank ^ 1 ][ DestReg ( c ) ].value = registers[ bank ][ SrcReg ( c ) ].value ;
        registers[ bank ^ 1 ][ DestReg ( c ) ].defined = true ;
        break ;

    case 0x02000 ... 0x02FFF : // AND sX, sY
    case 0x10B000 ... 0x10BFFF :
        registers[ bank ][ DestReg ( c ) ].value = registers[ bank ][ DestReg ( c ) ].value & registers[ bank ][ SrcReg ( c ) ].value ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        zero = registers[ bank ][ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;
    case 0x03000 ... 0x03FFF : // AND sX, K
    case 0x10A000 ... 0x10AFFF :
        registers[ bank ][ DestReg ( c ) ].value = registers[ bank ][ DestReg ( c ) ].value & Constant ( c ) ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        zero = registers[ bank ][ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;

    case 0x04000 ... 0x04FFF : // OR sX, sY
    case 0x10D000 ... 0x10DFFF :
        registers[ bank ][ DestReg ( c ) ].value = registers[ bank ][ DestReg ( c ) ].value | registers[ bank ][ SrcReg ( c ) ].value ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        zero = registers[ bank ][ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;
    case 0x05000 ... 0x05FFF : // OR sX, K
    case 0x10C000 ... 0x10CFFF :
        registers[ bank ][ DestReg ( c ) ].value = registers[ bank ][ DestReg ( c ) ].value | Constant ( c ) ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        zero = registers[ bank ][ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;

    case 0x06000 ... 0x06FFF : // XOR sX, sY
    case 0x10F000 ... 0x10FFFF :
        registers[ bank ][ DestReg ( c ) ].value = registers[ bank ][ DestReg ( c ) ].value ^ registers[ bank ][ SrcReg ( c ) ].value ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        zero = registers[ bank ][ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;
    case 0x07000 ... 0x07FFF : // XOR sX, K
    case 0x10E000 ... 0x10EFFF :
        registers[ bank ][ DestReg ( c ) ].value = registers[ bank ][ DestReg ( c ) ].value ^ Constant ( c ) ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        zero = registers[ bank ][ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;

    case 0x0C000 ... 0x0CFFF : // TEST sX, sY
    case 0x113000 ... 0x113FFF :
        t = registers[ bank ][ DestReg ( c ) ].value & registers[ bank ][ SrcReg ( c ) ].value ;
        zero = ( t & 0xFF ) == 0 ;
        t ^= t >> 4 ;
        t ^= t >> 2 ;
        t ^= t >> 1 ;
        carry = ( t & 1 ) == 1 ;
        break ;
    case 0x0D000 ... 0x0DFFF : // TEST sX, K
    case 0x112000 ... 0x112FFF :
        t = registers[ bank ][ DestReg ( c ) ].value & Constant ( c ) ;
        zero = ( t & 0xFF ) == 0 ;
        t ^= t >> 4 ;
        t ^= t >> 2 ;
        t ^= t >> 1 ;
        carry = ( t & 1 ) == 1 ;
        break ;
    case 0x0E000 ... 0x0EFFF : // TSTC sX, sY
        t = registers[ bank ][ DestReg ( c ) ].value & registers[ bank ][ SrcReg ( c ) ].value ;
        zero = ( ( t & 0xFF ) == 0 ) & zero ;
        t ^= t >> 4 ;
        t ^= t >> 2 ;
        t ^= t >> 1;
        carry = ( ( t & 1 ) == 1 ) ^ carry ;
        break ;
    case 0x0F000 ... 0x0FFFF : // TSTC sX, K
        t = registers[ bank ][ DestReg ( c ) ].value & Constant ( c ) ;
        zero = ( ( t & 0xFF ) == 0 ) & zero ;
        t ^= t >> 4 ;
        t ^= t >> 2 ;
        t ^= t >> 1 ;
        carry = ( ( t & 1 ) == 1 ) ^ carry ;
        break ;

    case 0x10000 ... 0x10FFF : // ADD sX, sY
    case 0x119000 ... 0x119FFF :
        t = registers[ bank ][ DestReg ( c ) ].value + registers[ bank ][ SrcReg ( c ) ].value ;
        registers[ bank ][ DestReg ( c ) ].value = t & 0xFF ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x11000 ... 0x11FFF : // ADD sX, K
    case 0x118000 ... 0x118FFF :
        t = registers[ bank ][ DestReg ( c ) ].value + Constant ( c ) ;
        registers[ bank ][ DestReg ( c ) ].value = t & 0xFF ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;

    case 0x12000 ... 0x12FFF : // ADDC sX, sY
    case 0x11B000 ... 0x11BFFF :
        t = registers[ bank ][ DestReg ( c ) ].value + registers[ bank ][ SrcReg ( c ) ].value + ( carry ? 1 : 0 ) ;
        registers[ bank ][ DestReg ( c ) ].value = t & 0xFF ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        if ( c & 0x100000 )
            zero = ( t & 0xFF ) == 0 ;
        else
            zero = ( ( t & 0xFF ) == 0 ) & zero ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x13000 ... 0x13FFF : // ADDC sX, K
    case 0x11A000 ... 0x11AFFF :
        t = registers[ bank ][ DestReg ( c ) ].value + Constant ( c ) + ( carry ? 1 : 0 ) ;
        registers[ bank ][ DestReg ( c ) ].value = t & 0xFF ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        if ( c & 0x100000 )
            zero = ( t & 0xFF ) == 0 ;
        else
            zero = ( ( t & 0xFF ) == 0 ) & zero ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;

    case 0x18000 ... 0x18FFF : // SUB sX, sY
    case 0x11D000 ... 0x11DFFF :
        t = registers[ bank ][ DestReg ( c ) ].value - registers[ bank ][ SrcReg ( c ) ].value ;
        registers[ bank ][ DestReg ( c ) ].value = t & 0xFF ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x19000 ... 0x19FFF : // SUB sX, K
    case 0x11C000 ... 0x11CFFF :
        t = registers[ bank ][ DestReg ( c ) ].value - Constant ( c ) ;
        registers[ bank ][ DestReg ( c ) ].value = t & 0xFF ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;

    case 0x1A000 ... 0x1AFFF : // SUBC sX, sY
    case 0x11F000 ... 0x11FFFF :
        t = registers[ bank ][ DestReg ( c ) ].value - registers[ bank ][ SrcReg ( c ) ].value - ( carry ? 1 : 0 )  ;
        registers[ bank ][ DestReg ( c ) ].value = t & 0xFF ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        if ( c & 0x100000 )
            zero = ( t & 0xFF ) == 0 ;
        else
            zero = ( ( t & 0xFF ) == 0 ) & zero ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x1B000 ... 0x1BFFF : // SUBC sX, K
    case 0x11E000 ... 0x11EFFF :
        t = registers[ bank ][ DestReg ( c ) ].value - Constant ( c ) - ( carry ? 1 : 0 ) ;
        registers[ bank ][ DestReg ( c ) ].value = t & 0xFF ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        if ( c & 0x100000 )
            zero = ( t & 0xFF ) == 0 ;
        else
            zero = ( ( t & 0xFF ) == 0 ) & zero ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;

    case 0x1C000 ... 0x1CFFF : // COMP sX, sY
    case 0x115000 ... 0x115FFF :
        t = registers[ bank ][ DestReg ( c ) ].value - registers[ bank ][ SrcReg ( c ) ].value ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x1D000 ... 0x1DFFF : // COMP sX, K
    case 0x114000 ... 0x114FFF :
        t = registers[ bank ][ DestReg ( c ) ].value - Constant ( c ) ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x1E000 ... 0x1EFFF : // CMPC sX, sY
        t = registers[ bank ][ DestReg ( c ) ].value - registers[ bank ][ SrcReg ( c ) ].value - ( carry ? 1 : 0 )  ;
        if ( c & 0x100000 )
            zero = ( t & 0xFF ) == 0 ;
        else
            zero = ( ( t & 0xFF ) == 0 ) & zero ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x1F000 ... 0x1FFFF : // CMPC sX, K
        t = registers[ bank ][ DestReg ( c ) ].value - Constant ( c ) - ( carry ? 1 : 0 ) ;
        if ( c & 0x100000 )
            zero = ( t & 0xFF ) == 0 ;
        else
            zero = ( ( t & 0xFF ) == 0 ) & zero ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;

    case 0x14000 ... 0x14FFF : // SHIFTs
    case 0x120000 ... 0x120FFF : // SHIFTs
        if ( c & 0xF0 ) {
            if ( bPB3 )
                return false ;
            registers[ bank ][ DestReg ( c ) ].value = 0x42 ;
            registers[ bank ][ DestReg ( c ) ].defined = true ;
        } else
            switch ( c & 0xF ) {
            case 0x2 : // RL sX
                t = registers[ bank ][ DestReg ( c ) ].value ;
                registers[ bank ][ DestReg ( c ) ].value = ( ( t << 1 ) | ( t >> 7 ) ) & 0xFF ;
                registers[ bank ][ DestReg ( c ) ].defined = true ;
                zero = registers[ bank ][ DestReg ( c ) ].value == 0 ;
                carry = ( ( t >> 7 ) & 1 ) == 1 ;
                break ;
            case 0x6 : // SL0 sX
                t = registers[ bank ][ DestReg ( c ) ].value ;
                registers[ bank ][ DestReg ( c ) ].value = ( ( t << 1 ) | 0 ) & 0xFF ;
                registers[ bank ][ DestReg ( c ) ].defined = true ;
                zero = registers[ bank ][ DestReg ( c ) ].value == 0 ;
                carry = ( ( t >> 7 ) & 1 ) == 1 ;
                break ;
            case 0x7 : // SL1 sX
                t = registers[ bank ][ DestReg ( c ) ].value ;
                registers[ bank ][ DestReg ( c ) ].value = ( ( t << 1 ) | 1 ) & 0xFF ;
                registers[ bank ][ DestReg ( c ) ].defined = true ;
                zero = registers[ bank ][ DestReg ( c ) ].value == 0 ;
                carry = ( ( t >> 7 ) & 1 ) == 1 ;
                break ;
            case 0x0 : // SLA sX
                t = registers[ bank ][ DestReg ( c ) ].value ;
                registers[ bank ][ DestReg ( c ) ].value = ( ( t << 1 ) | ( carry ? 1 : 0 ) ) & 0xFF ;
                registers[ bank ][ DestReg ( c ) ].defined = true ;
                zero = registers[ bank ][ DestReg ( c ) ].value == 0 ;
                carry = ( ( t >> 7 ) & 1 ) == 1 ;
                break ;
            case 0x4 : // SLX sX
                t = registers[ bank ][ DestReg ( c ) ].value ;
                registers[ bank ][ DestReg ( c ) ].value = ( ( t << 1 ) | ( t & 1 ) ) & 0xFF ;
                registers[ bank ][ DestReg ( c ) ].defined = true ;
                zero = registers[ bank ][ DestReg ( c ) ].value == 0 ;
                carry = ( ( t >> 7 ) & 1 ) == 1 ;
                break ;

            case 0xC : // RR sX
                t = registers[ bank ][ DestReg ( c ) ].value ;
                registers[ bank ][ DestReg ( c ) ].value = ( ( t >> 1 ) | ( t << 7 ) ) & 0xFF ;
                registers[ bank ][ DestReg ( c ) ].defined = true ;
                zero = registers[ bank ][ DestReg ( c ) ].value == 0 ;
                carry = ( t & 1 ) == 1 ;
                break ;
            case 0xE : // SR0 sX
                t = registers[ bank ][ DestReg ( c ) ].value ;
                registers[ bank ][ DestReg ( c ) ].value = ( ( t >> 1 ) | ( 0 << 7 ) ) & 0xFF ;
                registers[ bank ][ DestReg ( c ) ].defined = true ;
                zero = registers[ bank ][ DestReg ( c ) ].value == 0 ;
                carry = ( t & 1 ) == 1 ;
                break ;
            case 0xF : // SR1 sX
                t = registers[ bank ][ DestReg ( c ) ].value ;
                registers[ bank ][ DestReg ( c ) ].value = ( ( t >> 1 ) | ( 1 << 7 ) ) & 0xFF ;
                registers[ bank ][ DestReg ( c ) ].defined = true ;
                zero = registers[ bank ][ DestReg ( c ) ].value == 0 ;
                carry = ( t & 1 ) == 1 ;
                break ;
            case 0x8 : // SRA sX
                t = registers[ bank ][ DestReg ( c ) ].value ;
                registers[ bank ][ DestReg ( c ) ].value = ( ( t >> 1 ) | ( carry ? ( 1 << 7 ) : 0 ) ) & 0xFF ;
                registers[ bank ][ DestReg ( c ) ].defined = true ;
                zero = registers[ bank ][ DestReg ( c ) ].value == 0 ;
                carry = ( t & 1 ) == 1 ;
                break ;
            case 0xA : // SRX sX
                t = registers[ bank ][ DestReg ( c ) ].value ;
                registers[ bank ][ DestReg ( c ) ].value = ( ( t >> 1 ) | ( t & 0x80 ) ) & 0xFF ;
                registers[ bank ][ DestReg ( c ) ].defined = true ;
                zero = registers[ bank ][ DestReg ( c ) ].value == 0 ;
                carry = ( t  & 1 ) == 1 ;
                break ;

            default :
                return false ;
            }
        break ;

    case 0x22000 ... 0x22FFF : // JUMP
    case 0x134000 ... 0x134FFF :
        npc = Address12 ( c ) ;
        break ;

    case 0x32000 ... 0x32FFF : // JUMP Z
    case 0x135000 ... 0x1353FF :
        if ( zero )
            npc = Address12 ( c ) ;
        break ;

    case 0x36000 ... 0x36FFF : // JUMP NZ
    case 0x135400 ... 0x1357FF :
        if ( !zero )
            npc = Address12 ( c ) ;
        break ;

    case 0x3A000 ... 0x3AFFF : // JUMP C
    case 0x135800 ... 0x135BFF :
        if ( carry )
            npc = Address12 ( c ) ;
        break ;

    case 0x3E000 ... 0x3EFFF : // JUMP NC
    case 0x135C00 ... 0x135FFF :
        if ( !carry )
            npc = Address12 ( c ) ;
        break ;

    case 0x26000 ... 0x26FFF : // JUMP sX, sY
        return false ;

        break ;

    case 0x20000 ... 0x20FFF : // CALL
    case 0x130000 ... 0x130FFF : // CALL
        if ( sp > 30 )
            return false ;
        stack[ sp++ ].pc = npc ;
        npc = Address12 ( c ) ;
        break ;
    case 0x30000 ... 0x30FFF : // CALL Z
    case 0x131000 ... 0x1313FF : // CALL
        if ( sp > 30 )
            return false ;
        if ( zero ) {
            stack[ sp++ ].pc = npc ;
            npc = Address12 ( c ) ;
        }
        break ;
    case 0x34000 ... 0x34FFF : // CALL NZ
    case 0x131400 ... 0x1317FF : // CALL
        if ( sp > 30 )
            return false ;
        if ( ! zero ) {
            stack[ sp++ ].pc = npc ;
            npc = Address12 ( c ) ;
        }
        break ;
    case 0x38000 ... 0x38FFF : // CALL C
    case 0x131800 ... 0x131BFF : // CALL
        if ( sp > 30 )
            return false ;
        if ( carry ) {
            stack[ sp++ ].pc = npc ;
            npc = Address12 ( c ) ;
        }
        break ;
    case 0x3C000 ... 0x3CFFF : // CALL NC
    case 0x131C00 ... 0x131FFF : // CALL
        if ( sp > 30 )
            return false ;
        if ( ! carry ) {
            stack[ sp++ ].pc = npc ;
            npc = Address12 ( c ) ;
        }
        break ;
    case 0x24000 ... 0x24FFF : // CALL sX, sY
        if ( sp > 30 )
            return false ;

        return false ;
        break ;

    case 0x25000 : // RET
    case 0x12A000 :
        if ( sp <= 0 )
            return false ;
        npc = stack[ --sp ].pc ;
        break ;
    case 0x31000 : // RET Z
    case 0x12B000 :
        if ( zero ) {
            if ( sp <= 0 )
                return false ;
            npc = stack[ --sp ].pc ;
        }
        break ;
    case 0x35000 : // RET NZ
    case 0x12B400 :
        if ( ! zero ) {
                if ( sp <= 0 )
                    return false ;
            npc = stack[ --sp ].pc ;
        }
        break ;
    case 0x39000 : // RET C
    case 0x12B800 :
        if ( carry ) {
            if ( sp <= 0 )
                return false ;
            npc = stack[ --sp ].pc ;
        }
        break ;
    case 0x3D000 : // RET NC
    case 0x12BC00 :
        if ( ! carry ) {
            if ( sp <= 0 )
                return false ;
            npc = stack[ --sp ].pc ;
        }
        break ;
    case 0x21000 : // RET sX, KK
        if ( sp <= 0 )
            return false ;
        npc = stack[ --sp ].pc ;
        registers[ bank ][ DestReg ( c ) ].value = Constant ( c ) ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        break ;


    case 0x2E000 ... 0x2EFFF : // ST sX, sY
    case 0x12F000 ... 0x12FFFF : // ST sX, sY
        scratchpad[ registers[ bank ][ SrcReg ( c ) ].value ].value = registers[ bank ][ DestReg ( c ) ].value & 0xFF ;
        break ;
    case 0x2F000 ... 0x2FFFF : // ST sX, K
    case 0x12E000 ... 0x12EFFF : // ST sX, K
        scratchpad[ Constant ( c ) ].value = registers[ bank ][ DestReg ( c ) ].value & 0xFF ;
        break ;

    case 0x0A000 ... 0x0AFFF : // LD sX, sY
    case 0x107000 ... 0x107FFF : // LD sX, sY
        registers[ bank ][ DestReg ( c ) ].value = scratchpad[ registers[ bank ][ SrcReg ( c ) ].value ].value & 0xFF ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        break ;
    case 0x0B000 ... 0x0BFFF : // LD sX, K
    case 0x106000 ... 0x106FFF : // LD sX, K
        registers[ bank ][ DestReg ( c ) ].value = scratchpad[ Constant ( c ) ].value & 0xFF ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        break ;

    case 0x2C000 ... 0x2CFFF : // OUT sX, sY
    case 0x12D000 ... 0x12DFFF : // OUT sX, sY
        addr = registers[ bank ][ SrcReg ( c ) ].value ;
        if ( IO[ addr ].device == NULL )
            return false ;
         IO[ addr ].device->setValue( addr, registers[ bank ][ DestReg ( c ) ].value & 0xFF ) ;
        break ;
    case 0x2D000 ... 0x2DFFF : // OUT sX, K
    case 0x12C000 ... 0x12CFFF : // OUT sX, K
        addr = Constant ( c ) ;
        if ( IO[ addr ].device == NULL )
            return false ;
        IO[ addr ].device->setValue( addr, registers[ bank ][ DestReg ( c ) ].value & 0xFF ) ;
        break ;
    case 0x2B000 ... 0x2BFFF : // OUTK
        break ;

    case 0x08000 ... 0x08FFF : // IN sX, sY
    case 0x105000 ... 0x105FFF : // IN sX, sY
        addr = registers[ bank ][ SrcReg ( c ) ].value ;
        if ( IO[ addr ].device == NULL )
            return false ;
        registers[ bank ][ DestReg ( c ) ].value = IO[ addr ].device->getValue( addr ) & 0xFF ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        break ;
    case 0x09000 ... 0x09FFF : // IN sX, K
    case 0x104000 ... 0x104FFF : // IN sX, K
        addr = Constant ( c ) ;
        if ( IO[ addr ].device == NULL )
            return false ;
        registers[ bank ][ DestReg ( c ) ].value = IO[ addr ].device->getValue( addr ) & 0xFF ;
        registers[ bank ][ DestReg ( c ) ].defined = true ;
        break ;

    case 0x28000 : // DINT
    case 0x13C000 :
        enable = false ;
        break ;
    case 0x28001 : // EINT
    case 0x13C001 :
        enable = true ;
        break ;
    case 0x29000 :
    case 0x138000 :
        if ( sp <= 0 )
            return false ;
        npc = stack[ --sp ].pc ;
        zero = stack[ sp ].zero ;
        carry = stack[ sp ].carry ;
        enable = false ;
        break ;
    case 0x29001 :
    case 0x138001 :
        if ( sp <= 0 )
            return false ;
        npc = stack[ --sp ].pc ;
        zero = stack[ sp ].zero ;
        carry = stack[ sp ].carry ;
        enable = true ;
        break ;

    case 0x37000 : // BANK A
        bank = 0 ;
        break ;
    case 0x37001 : // BANK B
        bank = 1 ;
        break ;

    default :
      return false ;
    }

    Code[ pc ].count++ ;
    pc = npc ;          // only update when no error
    CycleCounter += 2 ; // 2 clock cycles per instruction
    return true ;
}



