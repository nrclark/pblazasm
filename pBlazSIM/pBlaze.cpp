/*
 *  Copyright � 2003..2014 : Henk van Kampen <henk@mediatronix.com>
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

#include "pBlaze.h"

Picoblaze::Picoblaze( void ) {
    scratchpad = NULL ;
    clearCode() ;
    initialize() ;
    for ( int io = 0 ; io < MAXIO ; io += 1 )
        IO[ io ].device = NULL ;
}

Picoblaze::~Picoblaze( void ) {
    // delete all IOs
    for ( int io = 0 ; io < MAXIO ; io += 1 )
        if ( IO[ io ].device != NULL ) {
            IODevice * d = IO[ io ].device ;
            // remove all copies
            for ( int i = io ; i < MAXIO ; i += 1 )
                if ( IO[ i ].device == d )
                    IO[ i ].device = NULL ;
            delete d ;
        }
    if ( scratchpad != NULL )
        delete scratchpad ;
}

bool Picoblaze::step() {
    error = erNone ;

    if ( ! enable )
        acknowledge = false ;
    else if ( interrupt ) {
        if ( sp > LIMSTK ) {
            error = erStackOverFlow ;
            return false ;
        }
        setStackPcValue( sp, pc ) ;
        stack[ sp ].carry = carry ? 1 : 0 ;
        stack[ sp ].zero = zero ? 1 : 0 ;
        stack[ sp ].bank = bank ;
        sp += 1 ;
        pc = intvect ;
        enable = false ;
        acknowledge = true ;
        return false ;
    }
    return true ;
}

void Picoblaze::clearCode( void ) {
    for ( int address = 0 ; address < MAXMEM ; address += 1 ) {
        Code[ address ].code = 0 ;
        Code[ address ].count = 0ll ;
        Code[ address ].line = 0 ;
        Code[ address ].breakpoint = true ;
    }
}

void Picoblaze::initialize( void ) {
    for ( int reg = 0 ; reg < MAXREG ; reg += 1 ) {
        registers[ 0 ][ reg ].value = 0 ;
        registers[ 0 ][ reg ].state = usUndef ;
        registers[ 1 ][ reg ].value = 0 ;
        registers[ 1 ][ reg ].state = usUndef ;
    }

    for ( int sp = 0 ; sp < MAXSTK ; sp += 1 ) {
        stack[ sp ].pc = 0 ;
        stack[ sp ].carry = -1 ;
        stack[ sp ].zero = -1 ;
        stack[ sp ].bank = -1 ;
        stack[ sp ].state = usUndef ;
    }

    reset() ;
}

void Picoblaze::reset( void ) {
    pc = 0 ;
    sp = 0 ;
    zero = false ;
    carry = false ;
    enable = false ;
    bank = 0 ;
    intvect = MAXMEM - 1 ;
    interrupt = false ;

    for ( int address = 0 ; address < MAXMEM ; address += 1 )
        Code[ address ].count = 0ll ;

    cycleCounter = 0ll ;
}


Picoblaze6::Picoblaze6() {
    scratchpad = new ScratchPad6() ;
}

bool Picoblaze6::step( void ) {
    if ( ! Picoblaze::step() )
        return error == erNone ; // an interrupt came inbetween?

    uint32_t c ;
    uint8_t value ;
    int addr = 0 ;

    c = Code[ pc ].code & 0x3FFFF ;
    int npc = pc + 1 ;

    switch ( c & 0x3F000 ) {
    case 0x00000 : // 0x00000 ... 0x00FFF : // MOVE sX, sY
        setRegister( bank, DestReg( c ), getRegister( bank, SrcReg( c ) ) ) ;
        break ;
    case 0x01000 : // 0x01000 ... 0x01FFF : // MOVE sX, kk
        setRegister( bank, DestReg( c ), Constant ( c ) ) ;
        break ;
    case 0x16000 : // 0x16000 ... 0x16FFF : // STAR sX, sY
        setRegister( bank ^ 1, DestReg( c ), getRegister( bank, SrcReg( c ) ) ) ;
        break ;
    case 0x17000 : // 0x17000 ... 0x17FFF : // STAR sX, kk  (was an undocumented instruction)
        setRegister( bank ^ 1, DestReg( c ), Constant ( c ) ) ;
        break ;

    case 0x02000 : // 0x02000 ... 0x02FFF : // AND sX, sY
        value = getRegister( bank, DestReg( c ) ) & getRegister( bank, SrcReg( c ) ) ;
        setRegister( bank, DestReg( c ), value ) ;
        setValueZero( value ) ;
        setCarry( false ) ;
        break ;
    case 0x03000 : // 0x03000 ... 0x03FFF : // AND sX, kk
        value = getRegister( bank, DestReg( c ) ) & Constant ( c ) ;
        setRegister( bank, DestReg( c ), value ) ;
        setValueZero( value ) ;
        setCarry( false ) ;
        break ;

    case 0x04000 : // 0x04000 ... 0x04FFF : // OR sX, sY
        value = getRegister( bank, DestReg( c ) ) | getRegister( bank, SrcReg( c ) ) ;
        setRegister( bank, DestReg( c ), value ) ;
        setValueZero( value ) ;
        setCarry( false ) ;
        break ;
    case 0x05000 : // 0x05000 ... 0x05FFF : // OR sX, kk
        value = getRegister( bank, DestReg( c ) ) | Constant ( c ) ;
        setRegister( bank, DestReg( c ), value ) ;
        setValueZero( value ) ;
        setCarry( false ) ;
        break ;

    case 0x06000 : // 0x06000 ... 0x06FFF : // XOR sX, sY
        value = getRegister( bank, DestReg( c ) ) ^ getRegister( bank, SrcReg( c ) ) ;
        setRegister( bank, DestReg( c ), value ) ;
        setValueZero( value ) ;
        setCarry( false ) ;
        break ;
    case 0x07000 : // 0x07000 ... 0x07FFF : // XOR sX, kk
        value = getRegister( bank, DestReg( c ) ) ^ Constant ( c ) ;
        setRegister( bank, DestReg( c ), value ) ;
        setValueZero( value ) ;
        setCarry( false ) ;
        break ;

    case 0x0C000 : // 0x0C000 ... 0x0CFFF : // TEST sX, sY
        value = getRegister( bank, DestReg( c ) ) & getRegister( bank, SrcReg( c ) ) ;
        setValueZero( value ) ;
        setParityCarry( value ) ;
        break ;
    case 0x0D000 : // 0x0D000 ... 0x0DFFF : // TEST sX, kk
        value = getRegister( bank, DestReg( c ) ) & Constant ( c ) ;
        setValueZero( value ) ;
        setParityCarry( value ) ;
        break ;
    case 0x0E000 : // 0x0E000 ... 0x0EFFF : // TSTC sX, sY
        value = getRegister( bank, DestReg( c ) ) & getRegister( bank, SrcReg( c ) ) ;
        updateValueZero( value ) ;
        updateParityCarry( value ) ;
        break ;
    case 0x0F000 : // 0x0F000 ... 0x0FFFF : // TSTC sX, kk
        value = getRegister( bank, DestReg( c ) ) & Constant ( c ) ;
        updateValueZero( value ) ;
        updateParityCarry( value ) ;
        break ;

    case 0x10000 : { // 0x10000 ... 0x10FFF : // ADD sX, sY
        uint32_t t = getRegister( bank, DestReg( c ) ) + getRegister( bank, SrcReg( c ) ) ;
        setRegister( bank, DestReg( c ), t & 0xFF )  ;
        setValueZero( t ) ;
        setCarry( ( ( t >> 8 ) & 1 ) == 1 ) ;
    }
        break ;
    case 0x11000 : { // 0x11000 ... 0x11FFF : // ADD sX, kk
        uint32_t t = getRegister( bank, DestReg( c ) ) + Constant ( c ) ;
        setRegister( bank, DestReg( c ), t & 0xFF ) ;
        setValueZero( t ) ;
        setValueCarry( t ) ;
    }
        break ;

    case 0x12000 : { // 0x12000 ... 0x12FFF : // ADDC sX, sY
        uint32_t  t = getRegister( bank, DestReg( c ) ) + getRegister( bank, SrcReg( c ) ) + ( getCarry() ? 1 : 0 ) ;
        setRegister( bank, DestReg( c ), t & 0xFF ) ;
        updateValueZero( t ) ;
        setValueCarry( t ) ;
    }
        break ;
    case 0x13000 : { // 0x13000 ... 0x13FFF : // ADDC sX, kk
        uint32_t t = getRegister( bank, DestReg( c ) ) + Constant ( c ) + ( getCarry() ? 1 : 0 ) ;
        setRegister( bank, DestReg( c ), t & 0xFF ) ;
        updateValueZero( t ) ;
        setValueCarry( t ) ;
    }
        break ;

    case 0x18000 : { // 0x18000 ... 0x18FFF : // SUB sX, sY
        uint32_t t = getRegister( bank, DestReg( c ) ) - getRegister( getBank(), SrcReg( c ) ) ;
        setRegister( bank, DestReg( c ), t & 0xFF ) ;
        setValueZero( t ) ;
        setValueCarry( t ) ;
    }
        break ;
    case 0x19000 : { // 0x19000 ... 0x19FFF : // SUB sX, kk
        uint32_t  t = getRegister( bank, DestReg( c ) ) - Constant ( c ) ;
        setRegister( bank, DestReg( c ), t & 0xFF ) ;
        setValueZero( t ) ;
        setValueCarry( t ) ;
    }
        break ;

    case 0x1A000 : { // 0x1A000 ... 0x1AFFF : // SUBC sX, sY
        uint32_t t = getRegister( bank, DestReg( c ) ) - getRegister( bank, SrcReg( c ) ) - ( getCarry() ? 1 : 0 )  ;
        setRegister( bank, DestReg( c ), t & 0xFF ) ;
        updateValueZero( t ) ;
        setValueCarry( t ) ;
    }
        break ;
    case 0x1B000 : { // 0x1B000 ... 0x1BFFF : // SUBC sX, kk
        uint32_t t = getRegister( bank, DestReg( c ) ) - Constant ( c ) - ( getCarry() ? 1 : 0 ) ;
        setRegister( bank, DestReg( c ), t & 0xFF ) ;
        updateValueZero( t ) ;
        setValueCarry( t ) ;
    }
        break ;

    case 0x1C000 : { // 0x1C000 ... 0x1CFFF : // COMP sX, sY
        uint32_t t = getRegister( bank, DestReg( c ) ) - getRegister( bank, SrcReg( c ) ) ;
        setValueZero( t ) ;
        setValueCarry( t ) ;
    }
        break ;
    case 0x1D000 : { // 0x1D000 ... 0x1DFFF : // COMP sX, K
        uint32_t t = getRegister( bank, DestReg( c ) ) - Constant ( c ) ;
        setValueZero( t ) ;
        setValueCarry( t ) ;
    }
        break ;
    case 0x1E000 : { // 0x1E000 ... 0x1EFFF : // CMPC sX, sY
        uint32_t t = getRegister( bank, DestReg( c ) ) - getRegister( bank, SrcReg( c ) ) - ( getCarry() ? 1 : 0 )  ;
        updateValueZero( t ) ;
        setValueCarry( t ) ;
    }
        break ;
    case 0x1F000 : { // 0x1F000 ... 0x1FFFF : // CMPC sX, kk
        uint32_t t = getRegister( bank, DestReg( c ) ) - Constant ( c ) - ( getCarry() ? 1 : 0 ) ;
        updateValueZero( t ) ;
        setValueCarry( t ) ;
    }
        break ;

    case 0x14000 : // 0x14000 ... 0x14FFF : // SHIFTs
        if ( c & 0xF0 ) {
            setRegister( bank, DestReg( c ), hwbuild ) ;
        } else
            switch ( c & 0xF ) {
            case 0x2 : { // RL sX
                uint32_t t = getRegister( bank, DestReg( c ) ) ;
                value = ( ( t << 1 ) | ( t >> 7 ) ) & 0xFF ;
                setRegister( bank, DestReg( c ), value ) ;
                setValueZero( value ) ;
                setCarry( ( ( t >> 7 ) & 1 ) == 1 ) ;
            }
                break ;
            case 0x6 : { // SL0 sX
                uint32_t t = getRegister( bank, DestReg( c ) ) ;
                value = ( ( t << 1 ) | 0 ) & 0xFF ;
                setRegister( bank, DestReg( c ), value )  ;
                setValueZero( value ) ;
                setCarry( ( ( t >> 7 ) & 1 ) == 1 ) ;
            }
                break ;
            case 0x7 : { // SL1 sX
                uint32_t t = getRegister( bank, DestReg( c ) ) ;
                value = ( ( t << 1 ) | 1 ) & 0xFF ;
                setRegister( bank, DestReg( c ), value ) ;
                setValueZero( value ) ;
                setCarry( ( ( t >> 7 ) & 1 ) == 1 ) ;
            }
                break ;
            case 0x0 : { // SLA sX
                uint32_t t = getRegister( bank, DestReg( c ) ) ;
                value = ( ( t << 1 ) | ( getCarry() ? 1 : 0 ) ) & 0xFF ;
                setRegister( bank, DestReg( c ), value ) ;
                setValueZero( value ) ;
                setCarry( ( ( t >> 7 ) & 1 ) == 1 ) ;
            }
                break ;
            case 0x4 : { // SLX sX
                uint32_t t = getRegister( bank, DestReg( c ) ) ;
                value = ( ( t << 1 ) | ( t & 1 ) ) & 0xFF ;
                setRegister( bank, DestReg( c ), value ) ;
                setValueZero( value ) ;
                setCarry( ( ( t >> 7 ) & 1 ) == 1 ) ;
            }
                break ;

            case 0xC : { // RR sX
                uint32_t t = getRegister( bank, DestReg( c ) ) ;
                value = ( ( t >> 1 ) | ( t << 7 ) ) & 0xFF ;
                setRegister( bank, DestReg( c ), value ) ;
                setValueZero( value ) ;
                setCarry( ( t & 1 ) == 1 ) ;
            }
                break ;
            case 0xE : { // SR0 sX
                uint32_t t = getRegister( bank, DestReg( c ) ) ;
                value = ( ( t >> 1 ) | ( 0 << 7 ) ) & 0xFF ;
                setRegister( bank, DestReg( c ), value ) ;
                setValueZero( value ) ;
                setCarry( ( t & 1 ) == 1 ) ;
            }
                break ;
            case 0xF : { // SR1 sX
                uint32_t t = getRegister( bank, DestReg( c ) ) ;
                value = ( ( t >> 1 ) | ( 1 << 7 ) ) & 0xFF ;
                setRegister( bank, DestReg( c ), value ) ;
                setValueZero( value ) ;
                setCarry( ( t & 1 ) == 1 ) ;
            }
                break ;
            case 0x8 : { // SRA sX
                uint32_t t = getRegister( bank, DestReg( c ) ) ;
                value = ( ( t >> 1 ) | ( getCarry() ? ( 1 << 7 ) : 0 ) ) & 0xFF ;
                setRegister( bank, DestReg( c ), value ) ;
                setValueZero( value ) ;
                setCarry( ( t & 1 ) == 1 ) ;
            }
                break ;
            case 0xA : { // SRX sX
                uint32_t t = getRegister( bank, DestReg( c ) ) ;
                value = ( ( t >> 1 ) | ( t & 0x80 ) ) & 0xFF ;
                setRegister( bank, DestReg( c ), value ) ;
                setValueZero( value ) ;
                setCarry( ( t & 1 ) == 1 ) ;
            }
                break ;

            default :
                error = erBadIntruction ;
                return false ;
            }
        break ;

    case 0x22000 : // 0x22000 ... 0x22FFF : // JUMP
        npc = Address ( c ) ;
        break ;

    case 0x23000 : // 0x23000 ... 0x22FFF : // BREAK
        error = erBreak ;
        return false ;

    case 0x32000 : // 0x32000 ... 0x32FFF : // JUMP Z
        if ( getZero() )
            npc = Address ( c ) ;
        break ;
    case 0x36000 : // 0x36000 ... 0x36FFF : // JUMP NZ
        if ( ! getZero() )
            npc = Address ( c ) ;
        break ;
    case 0x3A000 : // 0x3A000 ... 0x3AFFF : // JUMP C
        if ( getCarry() )
            npc = Address ( c ) ;
        break ;
    case 0x3E000 : // 0x3E000 ... 0x3EFFF : // JUMP NC
        if ( ! getCarry() )
            npc = Address ( c ) ;
        break ;
    case 0x26000 : { // 0x26000 ... 0x26FFF : // JUMP sX, sY
            int sX = getRegister( bank, DestReg( c ) ) ;
            int sY = getRegister( bank, SrcReg( c ) ) ;
            npc = ( ( sX << 8 ) & 0x0F ) | sY ;
    }
        break ;

    case 0x20000 : // 0x20000 ... 0x20FFF : // CALL
        if ( ! pushPC( npc ) ) {
            error = erStackOverFlow ;
            return false ;
        }
        npc = Address ( c ) ;
        break ;
    case 0x30000 : // 0x30000 ... 0x30FFF : // CALL Z
        if ( getZero() ) {
            if ( ! pushPC( npc ) ) {
                error = erStackOverFlow ;
                return false ;
            }
            npc = Address ( c ) ;
        }
        break ;
    case 0x34000 : // 0x34000 ... 0x34FFF : // CALL NZ
        if ( ! getZero() ) {
            if ( ! pushPC( npc ) ) {
                error = erStackOverFlow ;
                return false ;
            }
            npc = Address ( c ) ;
        }
        break ;
    case 0x38000 : // 0x38000 ... 0x38FFF : // CALL C
        if ( getCarry() ) {
            if ( ! pushPC( npc ) ) {
                error = erStackOverFlow ;
                return false ;
            }
            npc = Address ( c ) ;
        }
        break ;
    case 0x3C000 : // 0x3C000 ... 0x3CFFF : // CALL NC
        if ( ! getCarry() ) {
            if ( ! pushPC( npc ) ) {
                error = erStackOverFlow ;
                return false ;
            }
            npc = Address ( c ) ;
        }
        break ;
    case 0x24000 : { // 0x24000 ... 0x24FFF : // CALL sX, sY
        if ( ! pushPC( npc ) ) {
            error = erStackOverFlow ;
            return false ;
        }
        int sX = getRegister( bank, DestReg( c ) ) ;
        int sY = getRegister( bank, SrcReg( c ) ) ;
        npc = ( ( sX & 0x0F ) << 8 ) | sY ;
    }
        break ;

    case 0x25000 : // RET
        npc = popPC() ;
        if ( npc < 0 ) {
            error = erStackUnderflow ;
            return false ;
        }
        break ;
    case 0x31000 : // RET Z
        if ( getZero() ) {
            npc = popPC() ;
            if ( npc < 0 ) {
                error = erStackUnderflow ;
                return false ;
            }
        }
        break ;
    case 0x35000 : // RET NZ
        if ( ! getZero() ) {
            npc = popPC() ;
            if ( npc < 0 ) {
                error = erStackUnderflow ;
                return false ;
            }
        }
        break ;
    case 0x39000 : // RET C
        if ( getCarry() ) {
            npc = popPC() ;
            if ( npc < 0 ) {
                error = erStackUnderflow ;
                return false ;
            }
        }
        break ;
    case 0x3D000 : // RET NC
        if ( ! getCarry() ) {
            npc = popPC() ;
            if ( npc < 0 ) {
                error = erStackUnderflow ;
                return false ;
            }
        }
        break ;
    case 0x21000 : // RET sX, KK
        npc = popPC() ;
        if ( npc < 0 ) {
            error = erStackUnderflow ;
            return false ;
        }
        setRegister( bank, DestReg( c ), Constant ( c ) ) ;
        break ;

    case 0x2E000 : // 0x2E000 ... 0x2EFFF : // ST sX, sY
        addr = getRegister( bank, SrcReg( c ) ) ; // + Offset ( c ) ;
        scratchpad->setValue( addr, getRegister( bank, DestReg( c ) ) ) ;
        break ;
    case 0x2F000 : // 0x2F000 ... 0x2FFFF : // ST sX, kk
        scratchpad->setValue( Constant ( c ), getRegister( bank, DestReg( c ) ) ) ;
        break ;

    case 0x0A000 : // 0x0A000 ... 0x0AFFF : // LD sX, sY
        addr = getRegister( bank, SrcReg( c ) ) ; // + Offset ( c ) ;
        value = scratchpad->getValue( addr ) & 0xFF ;
        setRegister( bank, DestReg( c ), value ) ;
        break ;
    case 0x0B000 : // 0x0B000 ... 0x0BFFF : // LD sX, kk
        value = scratchpad->getValue( Constant ( c ) ) & 0xFF ;
        setRegister( bank, DestReg( c ), value ) ;
        break ;

    case 0x2C000 : // 0x2C000 ... 0x2CFFF : // OUT sX, sY
        addr = getRegister( bank, SrcReg( c ) ) ; // + Offset ( c ) ;
        if ( IO[ addr ].device == NULL ) {
            error = erUndefinedIO ;
            return false ;
        }
        IO[ addr ].device->setValue( addr, getRegister( bank, DestReg( c ) ) ) ;
        break ;
    case 0x2D000 : // 0x2D000 ... 0x2DFFF : // OUT sX, kk
        addr = Constant ( c ) ;
        if ( IO[ addr ].device == NULL ) {
            error = erUndefinedIO ;
            return false ;
        }
        IO[ addr ].device->setValue( addr, getRegister( bank, DestReg( c ) ) ) ;
        break ;
    case 0x2B000 : // 0x2B000 ... 0x2BFFF : // OUTK : 2Bkkp OUTPUTK kk, p
        // assumes unified IO
        value = ( c >> 4 ) & 0xFF ;
        addr = c & 0x0F ;
        if ( IO[ addr ].device == NULL ) {
            error = erUndefinedIO ;
            return false ;
        }
        IO[ addr ].device->setValue( addr, value ) ;
        break ;

    case 0x08000 : // 0x08000 ... 0x08FFF : // IN sX, sY
        addr = getRegister( bank, SrcReg( c ) ) ; // + Offset ( c ) ;
        if ( IO[ addr ].device == NULL ) {
            error = erUndefinedIO ;
            return false ;
        }
        value = IO[ addr ].device->getValue( addr ) & 0xFF ;
        setRegister( bank, DestReg( c ), value ) ;
        break ;
    case 0x09000 : // 0x09000 ... 0x09FFF : // IN sX, kk
        addr = Constant ( c ) ;
        if ( IO[ addr ].device == NULL ) {
            error = erUndefinedIO ;
            return false ;
        }
        value = IO[ addr ].device->getValue( addr ) & 0xFF ;
        setRegister( bank, DestReg( c ), value ) ;
        break ;

    case 0x28000 : // DINT / EINT
    case 0x28001 : // EINT
        setEnable( ( c & 1 ) != 0 ) ;
        break ;

    case 0x29000 : // RETI
    case 0x29001 :
        if ( sp <= 0 ) {
            error = erStackUnderflow ;
            return false ;
        }
        sp -= 1 ;
        npc = stack[ sp ].pc ;
        if ( stack[ sp ].bank < 0 ) {
            error = erIllegalRETI ;
            return false ;
        }
        setZero( stack[ sp ].zero == 1 ) ;
        setCarry( stack[ sp ].carry == 1 ) ;
        setBank( stack[ sp ].bank ) ;
        setEnable( ( c & 1 ) != 0 ) ;
        break ;

    case 0x37000 : // BANK A
    case 0x37001 : // BANK B
        setBank( c & 1 ) ;
        break ;

    default :
        error = erBadIntruction ;
        return false ;
    }

    Code[ pc ].count++ ;
    pc = npc & ( MAXMEM - 1 ) ;  // only update when no error
    cycleCounter += 2 ; // 2 clock cycles per instruction
    return true ;
}


Picoblaze3::Picoblaze3() {
    scratchpad = new ScratchPad3() ;
}

bool Picoblaze3::step ( void ) {
    if ( ! Picoblaze::step() )
        return error == erNone ;  // an interrupt came inbetween?

    uint32_t c, t ;
    int addr = 0 ;

    c = Code[ pc ].code & 0x3FFFF ;
    int npc = pc + 1 ;

    switch ( c & 0x3F000 ) {
    case 0x01000 : // 0x101000 ... 0x101FFF : // MOVE sX, sY
        registers[ 0 ][ DestReg ( c ) ].value = registers[ 0 ][ SrcReg ( c ) ].value ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        break ;
    case 0x00000 : // 0x100000 ... 0x100FFF : // MOVE sX, K
        registers[ 0 ][ DestReg ( c ) ].value = Constant ( c ) ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        break ;

    case 0x0B000 : // 0x10B000 ... 0x10BFFF : // AND sX, sY
        registers[ 0 ][ DestReg ( c ) ].value = registers[ 0 ][ DestReg ( c ) ].value & registers[ 0 ][ SrcReg ( c ) ].value ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        zero = registers[ 0 ][ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;
    case 0x0A000 : // 0x10A000 ... 0x10AFFF : // AND sX, K
        registers[ 0 ][ DestReg ( c ) ].value = registers[ 0 ][ DestReg ( c ) ].value & Constant ( c ) ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        zero = registers[ 0 ][ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;

    case 0x0D000 : // 0x10D000 ... 0x10DFFF : // OR sX, sY
        registers[ 0 ][ DestReg ( c ) ].value = registers[ 0 ][ DestReg ( c ) ].value | registers[ 0 ][ SrcReg ( c ) ].value ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        zero = registers[ 0 ][ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;
    case 0x0C000 : // 0x10C000 ... 0x10CFFF : // OR sX, K
        registers[ 0 ][ DestReg ( c ) ].value = registers[ 0 ][ DestReg ( c ) ].value | Constant ( c ) ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        zero = registers[ 0 ][ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;

    case 0x0F000 : // 0x10F000 ... 0x10FFFF : // XOR sX, sY
        registers[ 0 ][ DestReg ( c ) ].value = registers[ 0 ][ DestReg ( c ) ].value ^ registers[ 0 ][ SrcReg ( c ) ].value ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        zero = registers[ 0 ][ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;
    case 0x0E000 : // 0x10E000 ... 0x10EFFF : // XOR sX, K
        registers[ 0 ][ DestReg ( c ) ].value = registers[ 0 ][ DestReg ( c ) ].value ^ Constant ( c ) ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        zero = registers[ 0 ][ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;

    case 0x13000 : // 0x113000 ... 0x113FFF : // TEST sX, sY
        t = registers[ 0 ][ DestReg ( c ) ].value & registers[ 0 ][ SrcReg ( c ) ].value ;
        zero = ( t & 0xFF ) == 0 ;
        t ^= t >> 4 ;
        t ^= t >> 2 ;
        t ^= t >> 1 ;
        carry = ( t & 1 ) == 1 ;
        break ;
    case 0x12000 : // 0x112000 ... 0x112FFF : // TEST sX, K
        t = registers[ 0 ][ DestReg ( c ) ].value & Constant ( c ) ;
        zero = ( t & 0xFF ) == 0 ;
        t ^= t >> 4 ;
        t ^= t >> 2 ;
        t ^= t >> 1 ;
        carry = ( t & 1 ) == 1 ;
        break ;

    case 0x19000 : // 0x119000 ... 0x119FFF : // ADD sX, sY
        t = registers[ 0 ][ DestReg ( c ) ].value + registers[ 0 ][ SrcReg ( c ) ].value ;
        registers[ 0 ][ DestReg ( c ) ].value = t & 0xFF ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x18000 : // 0x118000 ... 0x118FFF : // ADD sX, K
        t = registers[ 0 ][ DestReg ( c ) ].value + Constant ( c ) ;
        registers[ 0 ][ DestReg ( c ) ].value = t & 0xFF ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;

    case 0x1B000 : // 0x11B000 ... 0x11BFFF : // ADDC sX, sY
        t = registers[ 0 ][ DestReg ( c ) ].value + registers[ 0 ][ SrcReg ( c ) ].value + ( carry ? 1 : 0 ) ;
        registers[ 0 ][ DestReg ( c ) ].value = t & 0xFF ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x1A000 : // 0x11A000 ... 0x11AFFF : // ADDC sX, K
        t = registers[ 0 ][ DestReg ( c ) ].value + Constant ( c ) + ( carry ? 1 : 0 ) ;
        registers[ 0 ][ DestReg ( c ) ].value = t & 0xFF ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
            zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;

    case 0x1D000 : // 0x11D000 ... 0x11DFFF : // SUB sX, sY
        t = registers[ 0 ][ DestReg ( c ) ].value - registers[ 0 ][ SrcReg ( c ) ].value ;
        registers[ 0 ][ DestReg ( c ) ].value = t & 0xFF ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x1C000 : // 0x11C000 ... 0x11CFFF : // SUB sX, K
        t = registers[ 0 ][ DestReg ( c ) ].value - Constant ( c ) ;
        registers[ 0 ][ DestReg ( c ) ].value = t & 0xFF ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;

    case 0x11F000 : // 0x11F000 ... 0x11FFFF : // SUBC sX, sY
        t = registers[ 0 ][ DestReg ( c ) ].value - registers[ 0 ][ SrcReg ( c ) ].value - ( carry ? 1 : 0 )  ;
        registers[ 0 ][ DestReg ( c ) ].value = t & 0xFF ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x1E000 : // 0x11E000 ... 0x11EFFF : // SUBC sX, K
        t = registers[ 0 ][ DestReg ( c ) ].value - Constant ( c ) - ( carry ? 1 : 0 ) ;
        registers[ 0 ][ DestReg ( c ) ].value = t & 0xFF ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;

    case 0x15000 : // 0x115000 ... 0x115FFF : // COMP sX, sY
        t = registers[ 0 ][ DestReg ( c ) ].value - registers[ 0 ][ SrcReg ( c ) ].value ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x14000 : // 0x114000 ... 0x114FFF : // COMP sX, K
        t = registers[ 0 ][ DestReg ( c ) ].value - Constant ( c ) ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;

    case 0x20000 : // 0x120000 ... 0x120FFF : // SHIFTs
        if ( c & 0xF0 ) {
            return false ;
        } else
            switch ( c & 0xF ) {
            case 0x2 : // RL sX
                t = registers[ 0 ][ DestReg ( c ) ].value ;
                registers[ 0 ][ DestReg ( c ) ].value = ( ( t << 1 ) | ( t >> 7 ) ) & 0xFF ;
                registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
                zero = registers[ 0 ][ DestReg ( c ) ].value == 0 ;
                carry = ( ( t >> 7 ) & 1 ) == 1 ;
                break ;
            case 0x6 : // SL0 sX
                t = registers[ 0 ][ DestReg ( c ) ].value ;
                registers[ 0 ][ DestReg ( c ) ].value = ( ( t << 1 ) | 0 ) & 0xFF ;
                registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
                zero = registers[ 0 ][ DestReg ( c ) ].value == 0 ;
                carry = ( ( t >> 7 ) & 1 ) == 1 ;
                break ;
            case 0x7 : // SL1 sX
                t = registers[ 0 ][ DestReg ( c ) ].value ;
                registers[ 0 ][ DestReg ( c ) ].value = ( ( t << 1 ) | 1 ) & 0xFF ;
                registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
                zero = registers[ 0 ][ DestReg ( c ) ].value == 0 ;
                carry = ( ( t >> 7 ) & 1 ) == 1 ;
                break ;
            case 0x0 : // SLA sX
                t = registers[ 0 ][ DestReg ( c ) ].value ;
                registers[ 0 ][ DestReg ( c ) ].value = ( ( t << 1 ) | ( carry ? 1 : 0 ) ) & 0xFF ;
                registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
                zero = registers[ 0 ][ DestReg ( c ) ].value == 0 ;
                carry = ( ( t >> 7 ) & 1 ) == 1 ;
                break ;
            case 0x4 : // SLX sX
                t = registers[ 0 ][ DestReg ( c ) ].value ;
                registers[ 0 ][ DestReg ( c ) ].value = ( ( t << 1 ) | ( t & 1 ) ) & 0xFF ;
                registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
                zero = registers[ 0 ][ DestReg ( c ) ].value == 0 ;
                carry = ( ( t >> 7 ) & 1 ) == 1 ;
                break ;

            case 0xC : // RR sX
                t = registers[ 0 ][ DestReg ( c ) ].value ;
                registers[ 0 ][ DestReg ( c ) ].value = ( ( t >> 1 ) | ( t << 7 ) ) & 0xFF ;
                registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
                zero = registers[ 0 ][ DestReg ( c ) ].value == 0 ;
                carry = ( t & 1 ) == 1 ;
                break ;
            case 0xE : // SR0 sX
                t = registers[ 0 ][ DestReg ( c ) ].value ;
                registers[ 0 ][ DestReg ( c ) ].value = ( ( t >> 1 ) | ( 0 << 7 ) ) & 0xFF ;
                registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
                zero = registers[ 0 ][ DestReg ( c ) ].value == 0 ;
                carry = ( t & 1 ) == 1 ;
                break ;
            case 0xF : // SR1 sX
                t = registers[ 0 ][ DestReg ( c ) ].value ;
                registers[ 0 ][ DestReg ( c ) ].value = ( ( t >> 1 ) | ( 1 << 7 ) ) & 0xFF ;
                registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
                zero = registers[ 0 ][ DestReg ( c ) ].value == 0 ;
                carry = ( t & 1 ) == 1 ;
                break ;
            case 0x8 : // SRA sX
                t = registers[ 0 ][ DestReg ( c ) ].value ;
                registers[ 0 ][ DestReg ( c ) ].value = ( ( t >> 1 ) | ( carry ? ( 1 << 7 ) : 0 ) ) & 0xFF ;
                registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
                zero = registers[ 0 ][ DestReg ( c ) ].value == 0 ;
                carry = ( t & 1 ) == 1 ;
                break ;
            case 0xA : // SRX sX
                t = registers[ 0 ][ DestReg ( c ) ].value ;
                registers[ 0 ][ DestReg ( c ) ].value = ( ( t >> 1 ) | ( t & 0x80 ) ) & 0xFF ;
                registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
                zero = registers[ 0 ][ DestReg ( c ) ].value == 0 ;
                carry = ( t  & 1 ) == 1 ;
                break ;

            default :
                return false ;
            }
        break ;

    case 0x34000 : // JUMP
        npc = Address ( c ) ;
        break ;

    case 0x35000 :
        switch ( c & 0xFFFC00 ) {
        case 0x35000 : // JUMP Z
            if ( zero )
                npc = Address ( c ) ;
            break ;
        case 0x35400 : // JUMP NZ
            if ( !zero )
                npc = Address ( c ) ;
            break ;
        case 0x35800 : // JUMP C
            if ( carry )
                npc = Address ( c ) ;
            break ;
        case 0x35C00 : // JUMP NC
            if ( !carry )
                npc = Address ( c ) ;
            break ;
        }
        break ;

    case 0x30000 : // CALL
        if ( sp > LIMSTK )
            return false ;
        stack[ sp++ ].pc = npc ;
        npc = Address ( c ) ;
        break ;

    case 0x31000 :
        switch ( c & 0x3FC00 ) {
        case 0x31000 : // CALL
                if ( sp > LIMSTK )
                    return false ;
                if ( zero ) {
                    stack[ sp++ ].pc = npc ;
                    npc = Address ( c ) ;
                }
                break ;
        case 0x31400 : // CALL
                if ( sp > LIMSTK )
                    return false ;
                if ( ! zero ) {
                    stack[ sp++ ].pc = npc ;
                    npc = Address ( c ) ;
                }
                break ;
        case 0x31800 : // CALL
                if ( sp > LIMSTK )
                    return false ;
                if ( carry ) {
                    stack[ sp++ ].pc = npc ;
                    npc = Address ( c ) ;
                }
                break ;
        case 0x31C00 : // CALL NC
            if ( sp > LIMSTK )
                return false ;
            if ( ! carry ) {
                stack[ sp++ ].pc = npc ;
                npc = Address ( c ) ;
            }
            break ;
        }
        break ;

    case 0x2A000 : // RET
        if ( sp <= 0 )
            return false ;
        npc = stack[ --sp ].pc ;
        break ;

    case 0x2B000:
        switch ( c & 0x3FC00 ) {
        case 0x2B000 : // RET Z
            if ( zero ) {
                if ( sp <= 0 )
                    return false ;
                npc = stack[ --sp ].pc ;
            }
            break ;
        case 0x2B400 : // RET NZ
            if ( ! zero ) {
                    if ( sp <= 0 )
                        return false ;
                npc = stack[ --sp ].pc ;
            }
            break ;
        case 0x2B800 : // RET C
            if ( carry ) {
                if ( sp <= 0 )
                    return false ;
                npc = stack[ --sp ].pc ;
            }
            break ;
        case 0x2BC00 : // RET NC
            if ( ! carry ) {
                if ( sp <= 0 )
                    return false ;
                npc = stack[ --sp ].pc ;
            }
            break ;
        }
        break ;

    case 0x2F000 : // 0x12F000 ... 0x12FFFF : // ST sX, sY
        addr = registers[ 0 ][ SrcReg ( c ) ].value ; // + Offset ( c ) ;
        scratchpad->setValue( addr, registers[ 0 ][ DestReg ( c ) ].value & 0xFF ) ;
        break ;
    case 0x2E000 : // 0x12E000 ... 0x12EFFF : // ST sX, K
        scratchpad->setValue( Constant ( c ), registers[ 0 ][ DestReg ( c ) ].value & 0xFF ) ;
        break ;

    case 0x07000 : // 0x107000 ... 0x107FFF : // LD sX, sY
        addr = registers[ 0 ][ SrcReg ( c ) ].value ; // + Offset ( c ) ;
        registers[ 0 ][ DestReg ( c ) ].value = scratchpad->getValue( addr ) & 0xFF ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        break ;
    case 0x06000 : // 0x106000 ... 0x106FFF : // LD sX, K
        registers[ 0 ][ DestReg ( c ) ].value = scratchpad->getValue( Constant ( c ) ) & 0xFF ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        break ;

    case 0x2D000 : // 0x12D000 ... 0x12DFFF : // OUT sX, sY
        addr = registers[ 0 ][ SrcReg ( c ) ].value ; // + Offset ( c ) ;
        if ( IO[ addr ].device == NULL )
            return false ;
         IO[ addr ].device->setValue( addr, registers[ 0 ][ DestReg ( c ) ].value & 0xFF ) ;
        break ;
    case 0x2C000 : // 0x12C000 ... 0x12CFFF : // OUT sX, K
        addr = Constant ( c ) ;
        if ( IO[ addr ].device == NULL )
            return false ;
        IO[ addr ].device->setValue( addr, registers[ 0 ][ DestReg ( c ) ].value & 0xFF ) ;
        break ;

    case 0x05000 : // 0x105000 ... 0x105FFF : // IN sX, sY
        addr = registers[ 0 ][ SrcReg ( c ) ].value ; // + Offset ( c ) ;
        if ( IO[ addr ].device == NULL )
            return false ;
        registers[ 0 ][ DestReg ( c ) ].value = IO[ addr ].device->getValue( addr ) & 0xFF ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        break ;
    case 0x04000 : // 0x104000 ... 0x104FFF : // IN sX, K
        addr = Constant ( c ) ;
        if ( IO[ addr ].device == NULL )
            return false ;
        registers[ 0 ][ DestReg ( c ) ].value = IO[ addr ].device->getValue( addr ) & 0xFF ;
        registers[ 0 ][ DestReg ( c ) ].state = usDefined ;
        break ;

    case 0x3C000 : // DINT / EINT
    case 0x3C001 : // EINT
        enable =  ( c & 1 ) != 0 ;
        break ;
    case 0x38000 : // RETI
    case 0x38001 :
        if ( sp <= 0 )
            return false ;
        npc = stack[ --sp ].pc ;
        zero = stack[ sp ].zero ;
        carry = stack[ sp ].carry ;
        enable = ( c & 1 ) != 0 ;
        break ;

    default :
      return false ;
    }

    Code[ pc ].count++ ;
    pc = npc & ( MAXMEM - 1 ) ;  // only update when no error
    cycleCounter += 2 ; // 2 clock cycles per instruction
    return true ;
}


Picoblaze7::Picoblaze7() {
    scratchpad = new ScratchPad7() ;
}

uint32_t ScratchPad7::getValue( uint32_t opcode ) {
    // opcode( 3 downto 1 ) selects one of 8 index registers
    IDX_t * idx = &index[ ( opcode >> 1 ) % MAXIDX ] ;
    uint32_t b = 0x42 ;

    switch ( opcode >> 4 ) {
    case 0 :
        // get upper and lower byte of index registers proper
        if ( opcode & 1 )
            b = ( idx->value >> 8 ) & 0xFF ;
        else
            b = ( idx->value >> 0 ) & 0xFF ;
    case 1 :
        // simple indirect ( *index )
        // value from ram[ index ]
        b = peekValue( idx->value ) ;
        break ;
    case 2 :
        // auto increment indirect ( *index++ )
        // value from ram[ index ]
        // index += 1
        b = peekValue( idx->value ) ;
        idx->value += 1 ;
        break ;
    case 3 :
        // auto decrement indirect ( *index-- )
        // value from ram[ index ]
        // index -= 1
        b = peekValue( idx->value ) ;
        idx->value -= 1 ;
        break ;
    }
    return b ;
}

uint32_t ScratchPad7::setValue( uint32_t opcode, uint32_t value ) {
    IDX_t * idx = &index[ ( opcode >> 1 ) % MAXIDX ] ;
    uint32_t b = value ;
    value &= 0xFF ;

    switch ( opcode >> 4 ) {
    case 0 :
        if ( opcode & 1 )
            // ST  s0, a0h
            b = idx->value = ( idx->value & 0x00FF ) | ( value << 8 ) ;
        else
            // ST  s1, a0l
            // setting lower byte, clears upper byte
            b = idx->value = value ;
        break ;
    case 1 :
        // ST  s0, a0
        b = pokeValue( idx->value, value ) ;
        break ;
    case 2 :
        // ST  s0, a0, 1
        b = pokeValue( idx->value, value ) ;
        idx->value += 1 ;
        break ;
    case 3 :
        // ST  s0, a0, -1
        b = pokeValue( idx->value, value ) ;
        idx->value -= 1 ;
        break ;

    case 14 :
        // INC  a0
        // index += 1 only, no data changed
        idx->value += 1 ;
        break ;
    case 15 :
        // DEC  a0
        // index -= 1 only, no data changed
        idx->value -= 1 ;
        break ;
    }
    return b ;
}




