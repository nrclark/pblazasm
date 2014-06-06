/*
 *  Copyright � 2003..2013 : Henk van Kampen <henk@mediatronix.com>
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

#include <stdint.h>

#define MAXMEM 4096
#define MAXSCR 256
#define MAXIO 256
#define MAXSTK 32
#define LIMSTK 30
#define MAXREG 16
#define MAXIDX 8

enum updateState { usUndef, usDefined, usChanged } ;
enum errorState { erNone, erBreakpoint, erBreak, erBadIntruction,
                  erStackOverFlow, erStackUnderflow, erUndefinedIO, erIllegalRETI } ;


class IODevice {
public:
    virtual ~IODevice(){}

    virtual uint32_t getValue ( uint32_t address ) { return address ; }
    virtual void setValue ( uint32_t address, uint32_t value ) { (void)address ; (void)value ; }
    virtual void update( void ) {}

    void * w ;

protected :
   int addr ;
} ;

class ScratchPad {
public:
    virtual ~ScratchPad() {}

    virtual void init( void ) = 0 ;
    virtual void clear( void ) = 0 ;
    virtual int getSize( void ) = 0 ;

    virtual uint32_t setValue( uint32_t opcode, uint32_t value ) {
        return pokeValue( opcode, value ) ;
    }
    virtual uint32_t getValue( uint32_t index ) {
        return peekValue( index ) ;
    }

    virtual enum updateState checkState( uint32_t cell ) = 0 ;

    virtual uint32_t peekValue( uint32_t cell ) = 0 ;
    virtual uint32_t pokeValue( uint32_t cell, uint32_t value ) = 0 ;

protected:
    typedef struct _index {
        uint32_t value ;
        bool defined ;
    } IDX_t ;

    typedef struct _data {
        uint8_t value ;
        enum updateState state ;
    } DATA_t ;
} ;

class  ScratchPad3 : public ScratchPad {
public:
    void init( void ) {
        for ( int cell = 0 ; cell < MAXSCR ; cell += 1 ) {
            ram[ cell ].value = 0 ;
            ram[ cell ].state = usUndef ;
        }
    }

    void clear( void ) {
        for ( int cell = 0 ; cell < MAXSCR ; cell += 1 ) {
            ram[ cell ].value = 0 ;
            ram[ cell ].state = usChanged ;
        }
    }
    int getSize( void ) {
        return MAXSCR ;
    }

    uint32_t peekValue( uint32_t cell ) {
        return ram[ cell ].value ;
    }

    enum updateState checkState( uint32_t cell ) {
        switch ( ram[ cell ].state ) {
        case usDefined :
            return usDefined ;
        case usChanged :
            ram[ cell ].state = usDefined ;
            return usChanged ;
        default:
            return usUndef ;
        }
    }

    uint32_t pokeValue( uint32_t cell, uint32_t value ) {
        if ( cell < MAXSCR ) {
            ram[ cell ].value = value ;
            ram[ cell ].state = usChanged ;
        }
        return value ;
    }

private:
    DATA_t ram[ MAXSCR ] ;
} ;

class  ScratchPad6 : public ScratchPad3 {
} ;

class ScratchPad7 : public ScratchPad {
public:
    void init( void ) {
        for ( int cell = 0 ; cell < MAXMEM ; cell += 1 ) {
            ram[ cell ].value = 0 ;
            ram[ cell ].state = usUndef ;
        }
        for ( int idx = 0 ; idx < MAXIDX ; idx += 1 ) {
            index[ idx ].value = 0 ;
        }
    }

    void clear( void ) {
        for ( int cell = 0 ; cell < MAXMEM ; cell += 1 ) {
            ram[ cell ].value = 0 ;
            ram[ cell ].state = usChanged ;
        }
        for ( int idx = 0 ; idx < MAXIDX ; idx += 1 ) {
            index[ idx ].value = 0 ;
        }
    }
    int getSize( void ) {
        return MAXMEM ;
    }

    enum updateState checkState( uint32_t cell ) {
        switch ( ram[ cell ].state ) {
        case usDefined :
            return usDefined ;
        case usChanged :
            ram[ cell ].state = usDefined ;
            return usChanged ;
        default:
            return usUndef ;
        }
    }

    uint32_t getValue(uint32_t opcode ) ;
    uint32_t setValue( uint32_t opcode, uint32_t value ) ;

    uint32_t peekValue( uint32_t cell )  {
        return ram[ cell ].value ;
    }
    uint32_t pokeValue( uint32_t cell, uint32_t value )  {
        if ( cell < MAXMEM ) {
            ram[ cell ].value = value ;
            ram[ cell ].state = usChanged ;
        }
        return value ;
    }

private:
    DATA_t ram[ MAXMEM ] ;
    IDX_t index[ MAXIDX ] ;
} ;



// Picoblaze
class Picoblaze {
    friend class IODevice ;

public:
    typedef struct _inst {
        uint32_t code ;
        unsigned int line ;
        uint64_t count ;
        bool breakpoint ;
    } INST_t ;

    typedef struct _stack {
        uint32_t pc ;
        int8_t bank ;
        int8_t zero ;
        int8_t carry ;
        enum updateState state ;
    } STACK_t ;

    typedef struct _register {
        uint8_t value ;
        enum updateState state ;
    } REG_t ;

    typedef struct _io {
        IODevice * device ;
    } IO_t ;

    Picoblaze( void ) ;
    virtual ~Picoblaze() ;

    virtual bool step ( void ) ;

    void setInterruptVector( int address ) {
        intvect = address ;
    }

    void setHWbuild( uint8_t val ) {
        hwbuild = val ;
    }

    void clearCode( void ) ;

    void initScratchpad( void )  {
        scratchpad->init() ;
    }

    inline bool onBarrier( void ) {
        return pc == barrier ;
    }

    inline void resetBarrier( void) {
        barrier = -1 ;
    }

    inline void setBarrier( void ) {
        barrier = pc + 1 ;
    }

    inline void setCeiling( void ) {
        if ( sp > 0 )
            barrier = stack[ sp - 1 ].pc ;
    }

    inline bool canLeave( void ) {
        return sp > 0 ;
    }

    inline bool onBreakpoint( void ) {
        return Code[ pc ].breakpoint ;
    }

    inline bool getBreakpoint( uint32_t address ) {
        return Code[ address ].breakpoint ;
    }

    inline void resetBreakpoint( int address ) {
        Code[ address ].breakpoint = false ;
    }

    inline void setBreakpoint( int address ) {
        Code[ address ].breakpoint = true ;
    }


    uint64_t getCodeCount( uint32_t address ) {
        return Code[ address ].count ;
    }

    void setCode( uint32_t address, uint32_t code, uint32_t line ){
        Code[ address ].code = code ;
        Code[ address ].line = line ;
        Code[ address ].breakpoint = false ;
        Code[ address ].count = 0ll ;
    }

    inline uint32_t getPcValue( void ) {
        return pc ;
    }

    inline uint32_t getSpValue( void ) {
        return sp ;
    }

    inline bool getZero() {
        return zero ;
    }
    inline void setZero( bool z ) {
        zero = z ;
    }
    inline void setValueZero( uint32_t v ) {
        zero = ( v & 0xFF ) == 0 ;
    }
    inline void updateValueZero( uint32_t v ) {
        zero &= ( v & 0xFF ) == 0 ;
    }

    inline bool getCarry() {
        return carry ;
    }
    inline void setCarry( bool c ) {
        carry = c ;
    }
    inline void setValueCarry( uint32_t v ) {
        carry = ( ( v >> 8 ) & 1 ) == 1 ;
    }
    inline void setParityCarry( uint32_t v ) {
        v ^= v >> 4 ;
        v ^= v >> 2 ;
        v ^= v >> 1 ;
        carry = ( v & 1 ) == 1 ;
    }
    inline void updateParityCarry( uint32_t v ) {
        v ^= v >> 4 ;
        v ^= v >> 2 ;
        v ^= v >> 1 ;
        carry ^= ( v & 1 ) == 1 ;
    }


    inline bool getEnable() {
        return enable ;
    }
    inline void setEnable( bool e ) {
        enable = e ;
    }

    inline int getBank() {
        return bank ;
    }
    inline void setBank( int b ) {
        bank = b ;
    }


    inline void setInterrupt( bool irq ) {
        interrupt = irq ;
    }

    inline bool getAcknowledge() {
        return acknowledge ;
    }

    inline int popPC( void ) {
        if ( sp <= 0 )
            return -1 ;
        return stack[ --sp ].pc ;
    }

    inline uint32_t getStackPcValue( uint32_t address ) {
        return stack[ address ].pc ;
    }

    inline Picoblaze::STACK_t getStackEntry( uint32_t address ) {
        return stack[ address ] ;
    }

    inline bool pushPC( uint32_t value ) {
        if ( sp > LIMSTK )
            return false ;
        stack[ sp ].pc = value ;
        stack[ sp ].zero = -1 ;
        stack[ sp ].carry = -1 ;
        stack[ sp ].bank = -1 ;
        stack[ sp++ ].state = usChanged ;
        return true ;
    }

    inline void setStackPcValue( uint32_t p, uint32_t value ) {
        stack[ p ].pc = value ;
        stack[ p ].state = usChanged ;
    }

    enum updateState checkStackEntryState( uint32_t p ) {
        switch ( stack[ p ].state ) {
        case usDefined :
            return usDefined ;
        case usChanged :
            stack[ p ].state = usDefined ;
            return usChanged ;
        default:
            return usUndef ;
        }
    }


    enum updateState checkScratchPadEntryState( uint32_t address ) {
        return scratchpad->checkState( address ) ;
    }
    int getScratchPadSize( void )  {
        return scratchpad->getSize() ;
    }
    inline uint32_t getScratchpadValue ( uint32_t cell ) {
        return scratchpad->peekValue( cell ) ;
    }
    inline uint32_t setScratchpadValue( uint32_t cell, uint32_t value ) {
        return scratchpad->pokeValue( cell, value ) ;
    }


    inline uint32_t getRegister ( int b, uint32_t reg ) {
        return registers[ b ][ reg ].value & 0xFF ;
    }
    inline void setRegister( int b, int reg, uint8_t value )  {
        registers[ b ][ reg ].value = value ;
        registers[ b ][ reg ].state = usChanged ;
    }
    bool isRegisterDefined( int reg ) {
        return registers[ bank ][ reg ].state != usUndef ;
    }
    enum updateState checkRegisterState( int reg ) {
        switch ( registers[ bank ][ reg ].state ) {
        case usDefined :
            return usDefined ;
        case usChanged :
            registers[ bank ][ reg ].state = usDefined ;
            return usChanged ;
        default:
            return usUndef ;
        }
    }

    void setIODevice ( void * w, int addr_l, int addr_h, IODevice * device ) {
        device->w = w ;
        for ( int addr = addr_l ; addr <= addr_h ; addr += 1 )
            if ( IO[ addr ].device == NULL )
                IO[ addr ].device = device ;
    }
    IODevice * getIODevice( uint32_t addr ) {
        return IO[ addr ].device ;
    }

    inline uint64_t getCycleCounter( void ) {
        return cycleCounter ;
    }

    inline enum errorState getErrorCode( void ) {
        return error ;
    }

    void initialize( void ) ;
    void reset( void ) ;


protected:
    uint32_t pc ;
    uint32_t sp ;
    int bank ;

    uint8_t hwbuild ;
    uint32_t intvect ;

    uint32_t barrier ;
    uint64_t cycleCounter ;
    enum errorState error ;

    INST_t Code[ MAXMEM ] ;
    ScratchPad * scratchpad ;
    STACK_t stack[ MAXSTK ] ;
    REG_t registers[ 2 ][ MAXREG ] ;
    IO_t IO[ MAXIO ] ;


    inline uint32_t DestReg ( const int code ) {
        return ( code >> 8 ) & 0xF ;
    }

    inline uint32_t SrcReg ( const int code ) {
        return ( code >> 4 ) & 0xF ;
    }

    inline uint32_t Offset ( const int code ) {
        return ( code >> 0 ) & 0xF ;
    }

    inline uint32_t Constant ( const int code ) {
        return code & 0xFF ;
    }

    virtual uint32_t Address ( const int code ) {
        return code & 0x3FF ;
    }

private:
    friend class Picoblaze3 ;
    bool carry, zero, enable, interrupt, acknowledge ;
} ;


class Picoblaze3 : public Picoblaze {
private:
protected:
    uint32_t Address ( const int code ) {
        return code & 0x3FF ;
    }
public:
    Picoblaze3( void ) ;
    bool step( void ) ;
} ;


class Picoblaze6 : public Picoblaze {
protected:
    virtual uint32_t Address ( const int code ) {
        return code & 0xFFF ;
    }
public:
    Picoblaze6( void ) ;
    bool step( void ) ;
} ;


class Picoblaze7 : public Picoblaze6 {
protected:
public:
    Picoblaze7( void ) ;
} ;

#endif // PBLAZE_H


