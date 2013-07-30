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

#ifndef PBLAZEIO_H
#define PBLAZEIO_H

#include <QStandardItem>
#include <QObject>

#include "pBlaze.h"

class QmtxPicoblaze ;

class SCRIPT : public QObject, public IODevice { Q_OBJECT
public:
   uint32_t getValue ( uint32_t address ) ;
   void setValue ( uint32_t address, uint32_t value ) ;

protected :
} ;

class UART : public QObject, public IODevice { Q_OBJECT
public:
    uint32_t getValue ( uint32_t address ) ;
    void setValue ( uint32_t address, uint32_t value ) ;

protected :
} ;

class CC : public QObject, public IODevice { Q_OBJECT
public:
    uint32_t getValue ( uint32_t address ) ;
    void setValue ( uint32_t address, uint32_t value ) ;

    QmtxPicoblaze * pBlaze ;

private:
    uint64_t TimeStamp ;
    uint64_t TimeDelta ;
} ;

class SBOX : public QObject, public IODevice { Q_OBJECT
public:
    uint32_t getValue ( uint32_t address ) ;
    void setValue ( uint32_t address, uint32_t value ) ;

private:
    uint32_t index ;
} ;

class LEDs : public QObject, public IODevice { Q_OBJECT
public:
    LEDs( void ) ;

    uint32_t getValue ( uint32_t address ) ;
    void setValue ( uint32_t address, uint32_t value ) ;
    void update( void ) ;

    void setItem ( uint32_t reg, QStandardItem * item ) ;

private:
    QStandardItem * leds[ 8 ] ;
    uint32_t rack ;

    QIcon * greenIcon ;
    QIcon * blackIcon ;
} ;

class QmtxPicoblaze : public QObject, public Picoblaze { Q_OBJECT
    friend class UART ;
    friend class CC ;
    friend class SBOX ;

public:
    void updateData( void ) ;
    void updateState( void ) ;
    void updateIO( void ) ;

    void setScratchpadValue ( uint32_t cell, uint32_t value ) ;
    void setRegisterValue ( uint32_t cell, uint32_t value ) ;

    void * getStateItem( uint32_t row ) {
        return stateItems[ row ] ;
    }

    void setStateItem ( uint32_t row, void * item ) {
        stateItems[ row ] = item ;
    }

private:
    void * stateItems[ 6 ] ;

    void updateScratchPad( void ) ;
    void updateStack( void ) ;
    void updateRegisters( void ) ;
} ;

#endif // PBLAZEIO_H


