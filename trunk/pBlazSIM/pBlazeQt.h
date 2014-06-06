/*
 *  Copyright � 2003..2012 : Henk van Kampen <henk@mediatronix.com>
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

#ifndef PBLAZEIO_H
#define PBLAZEIO_H

#include <QStandardItem>
#include <QObject>

#include "pBlaze.h"
#include "qmtxscriptcore.h"

enum pbState { psNone = 0, psConfigured, psReady, psBarrier, psBreakpoint, psRunning, psError } ;

class QmtxPicoblaze ;

class SCRIPT : public QObject, public IODevice {
    Q_OBJECT
public:
   uint32_t getValue ( uint32_t address ) ;
   void setValue ( uint32_t address, uint32_t value ) ;

protected :
} ;

//class UART : public QObject, public IODevice {
//   Q_OBJECT
//public:
//    uint32_t getValue ( uint32_t address ) ;
//    void setValue ( uint32_t address, uint32_t value ) ;

//protected :
//} ;

class CC : public QObject, public IODevice {
    Q_OBJECT
public:
    uint32_t getValue ( uint32_t address ) ;
    void setValue ( uint32_t address, uint32_t value ) ;

    QmtxPicoblaze * pBlaze ;

private:
    uint64_t TimeStamp ;
    uint64_t TimeDelta ;
} ;

class SBOX : public QObject, public IODevice {
    Q_OBJECT
public:
    uint32_t getValue ( uint32_t address ) ;
    void setValue ( uint32_t address, uint32_t value ) ;

private:
    uint32_t index ;
} ;

class QmtxPicoblaze : public QObject {
    Q_OBJECT

    friend class UART ;
    friend class CC ;
    friend class SBOX ;

public:
    QmtxPicoblaze( void ) ;
    ~QmtxPicoblaze( void ) ;

    enum coreType { ctNone = 0, ctPB3, ctPB6, ctPB7 } ;
    void setCore( coreType c ) ;

    void toggleZero( void ) {
        pico->setZero( ! pico->getZero() ) ;
    }
    void toggleCarry( void ) {
        pico->setCarry( ! pico->getCarry() ) ;
    }
    void toggleBank( void ) {
        pico->setBank( pico->getBank() == 0 ? 1 : 0 ) ;
    }
    void toggleEnable( void ) {
        pico->setEnable( ! pico->getEnable() ) ;
    }

    inline bool configured( void ) {
        return pico != NULL ;
    }

    void initialize( void ) {
        if ( pico != NULL )
            pico->initialize() ;
    }

    void reset( void ) {
        if ( configured() ) {
            pico->reset() ;
            emit updateUI( psReady ) ;
        } else
            emit updateUI( psNone ) ;
    }
    void setBarrier( void ) {
        if ( pico == NULL )
            return ;
        pico->setBarrier() ;
    }
    void setCeiling( void ) {
        if ( pico == NULL )
            return ;
        pico->setCeiling() ;
    }
    void resetBarrier( void ) {
        if ( pico == NULL )
            return ;
        pico->resetBarrier() ;
    }
    bool canLeave( void ) {
        if ( pico == NULL )
            return false ;
        return pico != NULL && pico->canLeave() ;
    }

    inline bool step( void ) {
        if ( ! pico->step() )
            return false ;
        if ( pico->getAcknowledge() )
            scriptCore->acknowledge() ;
        return true ;
    }

    inline bool onBreakpoint( void ) {
        if ( pico == NULL )
            return false ;
        return pico->onBreakpoint() ;
    }
    inline bool onBarrier( void ) {
        if ( pico == NULL )
            return false ;
        return pico->onBarrier() ;
    }

    void stop( void ) {
        if ( configured() ) {
            emit updateUI( psReady ) ;
        } else
            emit updateUI( psNone ) ;
    }

    bool getBreakpoint( uint32_t address ) {
        if ( pico == NULL )
            return false ;
        return pico->getBreakpoint( address ) ;
    }
    void resetBreakpoint( uint32_t address ) {
        if ( pico == NULL )
            return ;
        pico->resetBreakpoint( address ) ;
    }
    void setBreakpoint( uint32_t address ) {
        if ( pico == NULL )
            return ;
        pico->setBreakpoint( address ) ;
    }

    inline uint32_t getPcValue( void ) {
        if ( pico == NULL )
            return 0 ;
        return pico->getPcValue() ;
    }

    uint32_t getCodeCount( uint32_t address ) {
        if ( pico == NULL )
            return 0 ;
        return pico->getCodeCount( address ) ;
    }

    IODevice * getIODevice( uint32_t addr ) {
        if ( pico == NULL )
            return NULL ;
        return pico->getIODevice( addr ) ;
    }

    void setIODevice ( void * w, int addr_l, int addr_h, IODevice * device ) {
        if ( pico == NULL )
            return ;
        pico->setIODevice( w, addr_l, addr_h, device ) ;
    }

    void setScratchpadValue ( uint32_t cell, uint32_t value ) ;
    void setRegisterValue ( uint32_t cell, uint32_t value ) ;

    void * getStateItem( uint32_t row ) {
        return stateItems[ row ] ;
    }

    void setStateItem( uint32_t row, void * item ) {
        stateItems[ row ] = item ;
    }

    uint32_t getStackPcValue( uint32_t address ) {
        return pico->getStackPcValue( address ) ;
    }

    void * getStackItem( uint32_t row ) {
        return stackItems[ row ] ;
    }

    void setStackItem( uint32_t row, void * item ) {
        stackItems[ row ] = item ;
    }

    uint8_t getScratchpadValue( uint32_t cell ) {
        return pico->getScratchpadValue( cell ) ;
    }

    void * getScratchpadItem ( uint32_t cell ) {
        return scratchPadItems[ cell ] ;
    }

    void setScratchpadItem ( uint32_t cell, void * item ) {
        scratchPadItems[ cell ] = item  ;
    }

    uint8_t getRegisterValue( uint32_t cell ) {
        if ( pico == NULL )
            return 0 ;
        return pico->getRegister( pico->getBank(), cell ) ;
    }

    void * getRegisterItem( uint32_t reg ) {
        if ( pico == NULL )
            return NULL ;
        return registerItems[ reg ] ;
    }

    void setRegisterItem ( uint32_t reg, void * item ) {
        if ( pico == NULL )
            return ;
        registerItems[ reg ] = item ;
    }

    void * getCodeItem( uint32_t address ) {
        if ( pico == NULL )
            return NULL ;
        return codeItems[ address ] ;
    }

    void * getCurrentCodeItem( void ) {
        if ( pico == NULL )
            return NULL ;
        uint32_t pc = pico->getPcValue() ;
        if ( pc < MAXMEM )
            return getCodeItem( pc ) ;
        else
            return NULL ;
    }

    void clearCode( void ) {
        if ( pico == NULL )
            return ;
        pico->clearCode() ;
    }

    void setCodeItem( uint32_t address, uint32_t code, uint32_t line, void * item ) {
        if ( pico == NULL )
            return ;
        pico->setCode( address, code, line ) ;
        codeItems[ address ] = item ;
    }

    void initScratchpad( void ) {
        if ( pico == NULL )
            return ;
        pico->initScratchpad() ;
    }

    int getScratchPadSize( void )  {
        if ( pico == NULL )
            return 0 ;
        return pico->getScratchPadSize() ;
    }

    void setInterrupt( bool value ) {
        if ( pico == NULL )
            return ;
        pico->setInterrupt( value ) ;
    }

    void setIntVect( bool value ) {
        if ( pico == NULL )
            return ;
        pico->setInterruptVector( value ) ;
    }

    void setHWBuild( bool value ) {
        if ( pico == NULL )
            return ;
        pico->setHWbuild( value ) ;
    }

    uint64_t getCycleCounter( void ) {
        if ( pico == NULL )
            return 0 ;
        return pico->getCycleCounter() ;
    }

    void updateAll( void ) ;
    void updatePicoState( void ) ;

    QString getError( void ) ;

    QmtxScriptCore * scriptCore ;

signals:
    void updateUI( enum pbState state ) ;

private:
    Picoblaze * pico ;
    void * stateItems[ 6 ] ;
    void * scratchPadItems[ MAXSCR ] ;
    void * stackItems[ MAXSTK ] ;
    void * registerItems[ MAXREG ] ;
    void * codeItems[ MAXMEM ] ;

    void updateScratchPad( void ) ;
    void updateStack( void ) ;
    void updateRegisters( void ) ;
    void updateIO( void ) ;
} ;

#endif // PBLAZEIO_H


