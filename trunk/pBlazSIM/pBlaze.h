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

#ifndef PBLAZE_H
#define PBLAZE_H

#include <QStandardItem>

class Picoblaze ;

#define MAXMEM 4096
#define MAXSCR 256
#define MAXSTK 32
#define MAXREG 16

class IODevice : public QObject
 {
     Q_OBJECT

public:
    virtual ~IODevice(){}

    virtual uint32_t getValue ( uint32_t address ) { return 0 ; }
    virtual void setValue ( uint32_t address, uint32_t value ) {}
    virtual void update( void ) {}

    QObject * w ;

protected :
   int addr ;
} ;

class UART : public IODevice
{
     Q_OBJECT

 public:
    uint32_t getValue ( uint32_t address ) ;
    void setValue ( uint32_t address, uint32_t value ) ;

protected :
} ;

class CC : public IODevice
{
     Q_OBJECT

private:
    uint64_t TimeStamp ;
    uint64_t TimeDelta ;

public:
    uint32_t getValue ( uint32_t address ) ;
    void setValue ( uint32_t address, uint32_t value ) ;

    Picoblaze * pBlaze ;
} ;

class SBOX : public IODevice
{
     Q_OBJECT

 public:
    uint32_t getValue ( uint32_t address ) ;
    void setValue ( uint32_t address, uint32_t value ) ;

private:
    uint32_t index ;
} ;

class LEDs : public IODevice
{
     Q_OBJECT

 public:
    LEDs( void ) ;
    void setItem ( uint32_t reg, QStandardItem * item ) {
        leds[ reg ] = item ;
    }

    uint32_t getValue ( uint32_t address ) ;
    void setValue ( uint32_t address, uint32_t value ) ;
    void update( void ) ;

private:
    QStandardItem * leds[ 8 ] ;

    uint32_t rack ;

    QIcon * greenIcon ;
    QIcon * blackIcon ;
} ;


// Picoblaze
class Picoblaze : public QObject
 {
     Q_OBJECT

    friend class IODevice ;
    friend class UART ;
    friend class CC ;
    friend class SBOX ;

public:
    Picoblaze( void );
    ~Picoblaze();

    void clearCode( void ) ;
    void updateData( void ) ;
    void updateState( void ) ;
    void updateIO( void ) ;

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
        scratchpad[ cell ].value = value ;
    }

    QStandardItem * getCodeItem( uint32_t address ) {
        return Code[ address ].item ;
    }

    QStandardItem * getCurrentCodeItem( void ) {
        return Code[ pc ].item ;
    }

    uint64_t getCodeCount( uint32_t address ) {
        return Code[ address ].count ;
    }

    void setCodeItem( uint32_t address, uint32_t code, uint32_t line, QStandardItem * item ){
        Code[ address ].code = code ;
        Code[ address ].line = line ;
        Code[ address ].breakpoint = false ;
        Code[ address ].count = 0ll ;
        Code[ address ].item = item ;
    }

    void setRegisterItem ( uint32_t reg, QStandardItem * item ) {
        registers[ reg ].value = 0 ;
        registers[ reg ].item = item ;
    }

    uint32_t getPcValue( void ) {
        return pc ;
    }

    uint32_t getStackPcValue( uint32_t address ) {
        return stack[ address ].pc ;
    }

    void setStackItem ( uint32_t sp, QStandardItem * item ) {
        stack[ sp ].pc = 0 ;
        stack[ sp ].zero = false ;
        stack[ sp ].carry = false ;
        stack[ sp ].item = item ;
    }

    void setScratchpadItem ( uint32_t cell, QStandardItem * item ) {
        scratchpad[ cell ].value = 0 ;
        scratchpad[ cell ].item = item ;
    }

    void setScratchpadValue ( uint32_t cell, uint32_t value ) {
        scratchpad[ cell ].value = value ;
        if ( scratchpad[ cell ].item != NULL ) {
            QString s = QString("%1").arg( value, 2, 16 ).toUpper() ;
            scratchpad[ cell ].item->setData( s, Qt::DisplayRole ) ;
        }
    }

    uint32_t getScratchpadValue ( uint32_t cell ) {
        return scratchpad[ cell ].value ;
    }

    void setRegisterValue ( uint32_t cell, uint32_t value ) {
        registers[ cell ].value = value ;
        if ( registers[ cell ].item != NULL ) {
            QString s = QString("%1").arg( value, 2, 16 ).toUpper() ;
            registers[ cell ].item->setData( s, Qt::DisplayRole ) ;
        }
    }

    uint32_t getRegisterValue ( uint32_t cell ) {
        return registers[ cell ].value ;
    }

    void setStateItem ( uint32_t row, QStandardItem * item ) {
        stateItems[ row ] = item ;
    }

    void setIOdevice ( QObject * w, int addr_l, int addr_h, IODevice * device ) {
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
    void updateScratchPad( void ) ;
    void updateStack( void ) ;
    void updateRegisters( void ) ;

    typedef struct _inst {
        uint32_t code ;
        uint line ;
        bool breakpoint ;
        uint64_t count ;
        QStandardItem * item ;
    } INST_t ;

    typedef struct _stack {
        uint32_t pc ;
        bool zero ;
        bool carry ;
        QStandardItem * item ;
    } STACK_t ;

    typedef struct _register {
        uint32_t value ;
        QStandardItem * item ;
    } REG_t ;

    typedef struct _data {
        uint32_t value ;
        QStandardItem * item ;
    } DATA_t ;

    typedef struct _io {
        IODevice * device ;
        QStandardItem * item ;
    } IO_t ;

    uint64_t CycleCounter ;

    INST_t Code[ MAXMEM ] ;

    uint32_t pc, npc, barrier ;
    uint32_t sp, nsp ;

    DATA_t scratchpad[ MAXSCR ] ;
    STACK_t stack[ 32 ] ;
    REG_t registers[ 16 ] ;
    QStandardItem * stateItems[ 5 ] ;

    bool carry, zero, enable ;


    uint32_t DestReg ( const int code ) {
        return ( code >> 8 ) & 0xF ;
    }

    uint32_t SrcReg ( const int code ) {
        return ( code >> 4 ) & 0xF ;
    }

    uint32_t Constant ( const int code ) {
        return code & 0xFF ;
    }

    uint32_t Address12 ( const int code ) {
        return code & 0xFFF ;
    }

    IO_t IO[ MAXSCR ] ;
} ;



#endif // PBLAZE_H


