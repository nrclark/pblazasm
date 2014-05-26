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

#ifndef QMTXPICOTERM_H
#define QMTXPICOTERM_H

#include <QObject>
#include <QQueue>
#include <QKeyEvent>
#include <QDebug>
#include <QDateTime>

#include <stdint.h>

enum ptState { ptNone, ptDCS, ptDCS2, ptESC, ptESC2, ptESC3 } ;

class QmtxPicoTerm : public QObject
{
    Q_OBJECT
public:
    explicit QmtxPicoTerm(QObject *parent = 0);

    uint32_t getChar( void ) {
        return inQ.dequeue() ;
    }

    void putChar( char c ) {
        inQ.enqueue( c ) ;
    }

    bool isEmpty( void ) {
        return inQ.isEmpty() ;
    }

signals:
    void sendChar( uint32_t c ) ;
    void showChar( uint32_t c ) ;

    void clearScreen( void ) ;
    void dcsLog( QList<uint32_t> l ) ;

    void ledControl( int r, int g, int b ) ;
    void getVirtualSwitches( void ) ;

public slots:
    void receivedChar( uint32_t c ) ;

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    enum ptState state ;
    QQueue<uint32_t> inQ ;
    QList<char> dcsChars ;
    char escChar ;

} ;

#endif // QMTXPICOTERM_H
