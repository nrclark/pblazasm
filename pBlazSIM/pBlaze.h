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

#ifndef PBLAZE_H
#define PBLAZE_H

#define MAXMEM 4096
#define MAXSCR 256
#define MAXIO 256
#define MAXSTK 32
#define MAXREG 16

class IODevice {
public:
    virtual ~IODevice(){}

    virtual uint32_t getValue ( uint32_t address ) { return address ; }
    virtual void setValue ( uint32_t address, uint32_t value ) {}
    virtual void update( void ) {}

    void * w ;

protected :
   int addr ;
} ;


// Picoblaze
class Picoblaze {
    friend class IODevice ;
    friend class UART ;
    friend class CC ;
    friend class SBOX ;

public:
    Picoblaze( void );
    ~Picoblaze();

    void clearCode( void ) ;
    void clearScratchpad( void ) ;

    void setCore( bool bCore ) {
        bPB3 = bCore ;
    }

    bool onBarrier( void ) {
        return pc == barrier ;
    }

    void resetBarrier( void) {
        barrier = -1 ;
    }

    void setBarrier( void ) {
        barrier = pc + 1 ;
    }

    bool onBreakpoint( void ) {
        return Code[ pc ].breakpoint ;
    }

    bool getBreakpoint( uint32_t address ) {
        return Code[ address ].breakpoint ;
    }

    void resetBreakpoint( int address ) {
        Code[ address ].breakpoint = false ;
    }

    void setBreakpoint( int address ) {
        Code[ address ].breakpoint = true ;
    }

    void setScratchpadData( uint32_t cell, uint32_t value ) {
        if ( cell < MAXSCR )
            scratchpad[ cell ].value = value ;
    }

    void * getCodeItem( uint32_t address ) {
        return Code[ address ].item ;
    }

    void * getCurrentCodeItem( void ) {
        if ( pc < MAXMEM )
            return Code[ pc ].item ;
        else
            return NULL ;
    }

    uint64_t getCodeCount( uint32_t address ) {
        return Code[ address ].count ;
    }

    void setCodeItem( uint32_t address, uint32_t code, uint32_t line, void * item ){
        Code[ address ].code = code ;
        Code[ address ].line = line ;
        Code[ address ].breakpoint = false ;
        Code[ address ].count = 0ll ;
        Code[ address ].item = item ;
    }

    uint32_t getPcValue( void ) {
        return pc ;
    }

    uint32_t getSpValue( void ) {
        return sp ;
    }

    bool getZero() {
        return zero ;
    }

    bool getCarry() {
        return carry ;
    }

    bool getEnable() {
        return enable ;
    }

    int getBank() {
        return bank ;
    }

    uint32_t getStackPcValue( uint32_t address ) {
        return stack[ address ].pc ;
    }

    void * getStackItem ( uint32_t sp ) {
        return stack[ sp ].item ;
    }

    void setStackItem ( uint32_t sp, void * item ) {
        stack[ sp ].pc = 0 ;
        stack[ sp ].zero = false ;
        stack[ sp ].carry = false ;
        stack[ sp ].item = item ;
    }

    void * getScratchpadItem ( uint32_t cell ) {
        return scratchpad[ cell ].item ;
    }

    void setScratchpadItem ( uint32_t cell, void * item ) {
        scratchpad[ cell ].value = 0 ;
        scratchpad[ cell ].item = item ;
    }

    void setScratchpadValue ( uint32_t cell, uint32_t value ) {
        scratchpad[ cell ].value = value ;
    }

    uint32_t getScratchpadValue ( uint32_t cell ) {
        return scratchpad[ cell ].value ;
    }

    void * getRegisterItem( uint32_t reg ) {
        return registers[ 1 ][ reg ].item ;
    }

    void setRegisterItem ( uint32_t reg, void * item ) {
        registers[ 0 ][ reg ].value = 0 ;
        registers[ 0 ][ reg ].item = item ;
        registers[ 1 ][ reg ].value = 0 ;
        registers[ 1 ][ reg ].item = item ;
    }

    void setRegisterValue ( uint32_t cell, uint32_t value ) {
        registers[ bank ][ cell ].value = value ;
    }

    uint32_t getRegisterValue ( uint32_t cell ) {
        return registers[ bank ][ cell ].value ;
    }

    bool isRegisterDefined( int reg ) {
        return registers[ bank ][ reg ].defined ;
    }

    void setIODevice ( void * w, int addr_l, int addr_h, IODevice * device ) {
        device->w = w ;
        for ( int addr = addr_l ; addr <= addr_h ; addr +=1 )
            IO[ addr ].device = device ;
    }

    IODevice * getIODevice( int addr ) {
        return IO[ addr ].device ;
    }

    void initPB6( void ) ;
    void resetPB6 ( void ) ;
    bool stepPB6 ( void ) ;

private:
    typedef struct _inst {
        uint32_t code ;
        unsigned line ;
        bool breakpoint ;
        uint64_t count ;
        void * item ;
    } INST_t ;

    typedef struct _stack {
        uint32_t pc ;
        bool zero ;
        bool carry ;
        void * item ;
    } STACK_t ;

    typedef struct _register {
        uint32_t value ;
        bool defined ;
        void * item ;
    } REG_t ;

    typedef struct _data {
        uint32_t value ;
        void * item ;
    } DATA_t ;

    typedef struct _io {
        IODevice * device ;
        void * item ;
    } IO_t ;

    bool bPB3 ;
    uint64_t CycleCounter ;

    uint32_t pc, npc, barrier ;
    uint32_t sp, nsp ;
    int bank ;

    INST_t Code[ MAXMEM ] ;
    DATA_t scratchpad[ MAXSCR ] ;
    STACK_t stack[ 32 ] ;
    REG_t registers[ 2 ][ 16 ] ;
    IO_t IO[ MAXSCR ] ;

    bool carry, zero, enable ;

    inline uint32_t DestReg ( const int code ) {
        return ( code >> 8 ) & 0xF ;
    }

    inline uint32_t SrcReg ( const int code ) {
        return ( code >> 4 ) & 0xF ;
    }

    inline uint32_t Constant ( const int code ) {
        return code & 0xFF ;
    }

    inline uint32_t Address12 ( const int code ) {
        if ( bPB3 )
            return code & 0x3FF ;
        else
            return code & 0xFFF ;
    }
} ;

#endif // PBLAZE_H


