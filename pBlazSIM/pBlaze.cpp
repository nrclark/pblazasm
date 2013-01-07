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


#include "pBlaze.h"
#include "mainwindow.h"


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

void Picoblaze::updateScratchPad( void ) {
    for ( int scr = 0 ; scr < MAXSCR ; scr += 1 ) {
        int n = scratchpad[ scr ].value ;
        QString s = QString("%1").arg( n, 2, 16 ).toUpper() ;
        scratchpad[ scr ].item->setData( s, Qt::DisplayRole ) ;
    }
}

void Picoblaze::updateStack( void ) {
    QString s  ;

    for ( int p = 0 ; p < MAXSTK ; p += 1 ) {
        s = "" ;
        if ( p < sp ) {
            int n = stack[ p ].pc ;
            s = QString("%1").arg( n, 0, 16 ).toUpper() ;
            s = s.rightJustified( 3, '0' ) ;
        }
        stack[ p ].item->setData( s, Qt::DisplayRole ) ;
    }
}

void Picoblaze::updateRegisters( void ) {
    QString s  ;

    for ( int reg = 0 ; reg < MAXREG ; reg += 1 ) {
        if ( registers[ bank ][ reg ].defined ) {
            int n = registers[ bank ][ reg ].value ;
            s = QString("%1").arg( n, 0, 16 ).toUpper() ;
            s = s.rightJustified( 2, '0' ) ;
            registers[ bank ][ reg ].item->setData( s, Qt::DisplayRole ) ;
        } else
            registers[ bank ][ reg ].item->setData( "", Qt::DisplayRole ) ;
    }
}

void Picoblaze::updateState( void ) {
    QString s = QString("%1").arg( pc, 4, 16 ).toUpper() ;
    stateItems[ 0 ]->setData( s, Qt::DisplayRole ) ;
    s = QString("%1").arg( sp, 4 ) ;
    stateItems[ 1 ]->setData( s, Qt::DisplayRole ) ;
    s = QString("%1").arg( (int)zero, 4 ) ;
    stateItems[ 2 ]->setData( s, Qt::DisplayRole ) ;
    s = QString("%1").arg( (int)carry, 4 ) ;
    stateItems[ 3 ]->setData( s, Qt::DisplayRole ) ;
    s = QString("%1").arg( (int)enable, 4 ) ;
    stateItems[ 4 ]->setData( s, Qt::DisplayRole ) ;
    s = bank == 0 ? "   A" : bank == 1 ? "   B" : "  ???" ;
    stateItems[ 5 ]->setData( s, Qt::DisplayRole ) ;
}

void Picoblaze::updateIO( void )
{
    for ( int io = 0 ; io < MAXIO ; io += 1 )
        if ( IO[ io ].device != NULL )
            IO[ io ].device->update();
}

void Picoblaze::updateData( void ) {
    updateScratchPad() ;
    updateStack() ;
    updateRegisters() ;
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

// IO devices

// UART
uint32_t UART::getValue ( uint32_t address ) {
    if ( ( address & 1 ) == 0 )
        return ( (MainWindow *)w )->getUARTstatus() ;
    else
        return ( (MainWindow *)w )->getUARTdata() ;
}

void UART::setValue ( uint32_t address, uint32_t value ) {
    ( (MainWindow *)w )->setUARTdata( value ) ;
}

// Cyclecounter CC
uint32_t CC::getValue(uint32_t address)
{
    if ( ( address & 0x08 ) == 0 ) {
    uint64_t c = TimeStamp ;

        switch ( address & 0x07 ) {
        case 7 : c >>= 8 ;
        case 6 : c >>= 8 ;
        case 5 : c >>= 8 ;
        case 4 : c >>= 8 ;
        case 3 : c >>= 8 ;
        case 2 : c >>= 8 ;
        case 1 : c >>= 8 ;
        case 0 : ;
        }
        return c & 0xFF ;
    } else {
        uint64_t c = TimeDelta ;

        switch ( address & 0x07 ) {
        case 7 : c >>= 8 ;
        case 6 : c >>= 8 ;
        case 5 : c >>= 8 ;
        case 4 : c >>= 8 ;
        case 3 : c >>= 8 ;
        case 2 : c >>= 8 ;
        case 1 : c >>= 8 ;
        case 0 : ;
        }
        return c & 0xFF ;
    }
}

void CC::setValue(uint32_t address, uint32_t value)
{
    TimeDelta = pBlaze->CycleCounter - TimeStamp ;
    TimeStamp = pBlaze->CycleCounter ;
}

// SBOX, for Rijndael
uint32_t SBOX::getValue(uint32_t address)
{
    static const unsigned char SBox[256] = {
       0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
       0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
       0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
       0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
       0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
       0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
       0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
       0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
       0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
       0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
       0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
       0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
       0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
       0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
       0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
       0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
    } ;
    return SBox[ index & 0xFF ] ;
}

void SBOX::setValue( uint32_t address, uint32_t value )
{
    index = value ;
}

// LEDs
uint32_t LEDs::getValue(uint32_t address)
{
    return rack ;
}

void LEDs::setValue(uint32_t address, uint32_t value )
{
    rack = value ;
}

void LEDs::update( void )
{
    for ( int bits = 0 ; bits < 8 ; bits += 1 ) {
        if ( leds[ bits ] != NULL ) {
            // set or reset the breakpoint at that address
            if ( ( ( rack >> bits ) & 1 ) != 0 ) {
                leds[ bits ]->setIcon( *greenIcon ) ;
            } else {
                leds[ bits ]->setIcon( *blackIcon ) ;
            }
        }
    }
}

LEDs::LEDs()
{
    greenIcon = new QIcon(":/images/bullet_ball_glass_red.png");
    blackIcon = new QIcon(":/images/bullet_ball_glass.png");

    rack = 0 ;
}

// SCRIPT
uint32_t SCRIPT::getValue(uint32_t address) {
    return ( (MainWindow *)w )->getScriptValue( address ) ;
}

void SCRIPT::setValue(uint32_t address, uint32_t value) {
    ( (MainWindow *)w )->setScriptValue( address, value ) ;
}




