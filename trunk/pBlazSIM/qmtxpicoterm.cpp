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

#include "qmtxpicoterm.h"

//  D - Read Date string
//  d - Read Date value
//  G - Plot point in graphic display
//  g - Plot character in graphic display
//  h - Hide transaction window
//  L - LED display
//  N - Request a Pseudo Random Number
//  P - 'Ping' with PicoTerm version request
//  p - 'Ping'
//  q - Force PicoTerm to restart
//  Q - Force PicoTerm application to quit
//  R - Read default text file
//  r - Read any text file with auto prompt
//  S - Read switches
//  s - Set switches
//  T - Read Time string
//  t - Read Time value
//  V - Fill a box in the graphic display
//  v - Draw a line in the graphic display
//  W - Open a LOG file
//  w - Close the LOG file
//  7 - Seven segment display

QmtxPicoTerm::QmtxPicoTerm(QObject *parent) :
    QObject(parent)
{
    state = ptNone ;
}

void QmtxPicoTerm::receivedChar( uint32_t c ) {
    switch ( state ) {
    case ptNone :
        switch ( (uchar)c ) {
        case 0x90 : // DCS
            state = ptDCS ;
            break ;
        case 0x9C : // ST
            qDebug() << "lone ST" ;
            break ;
        case 0x1B : // ESC
            state = ptESC ;
            break ;
        default :
            if ( (char)c != '\n' )
                emit showChar( c ) ;
        }
        break ;
    case ptDCS :
        dcsChars.clear() ;
        switch ( (char)c ) {
        case 'p' :
        case 'P' :
        case 'T' :
        case 't' :
        case 'D' :
        case 'd' :
        case 'L' :
        case 'S' :
            dcsChars << (char)c ;
            state = ptDCS2 ;
            break ;
        default :
            qDebug() << c << '?' ;
            state = ptNone ;
        }
        break ;
    case ptDCS2 :
        if ( (uchar)c == 0x9C ) { // ST
            switch( dcsChars.first() ) {
            case 'p' :
                inQ << 0x90 << 'P' << 0x9C ;
                break ;
            case 'P' :
                inQ << 0x90 << 'p' << 'v' << '1' << '.' << '9' << '4' << 0x9C ;
                break ;
            case 'T' : {
                QDateTime UTC( QDateTime::currentDateTimeUtc() ) ;
                QDateTime local( UTC.toLocalTime() ) ;

                QByteArray time ;
                time.append( local.toString("Thh:mm:ss") ) ;
                inQ << 0x90 ;
                inQ << 'T' ;
                foreach( char t, time )
                    inQ << t ;
                inQ << 0x9C ;
            }
                break ;
            case 't' : {
                QTime time( QTime::currentTime() ) ;
                inQ << 0x90 ;
                inQ << 't' ;
                inQ << time.hour() ;
                inQ << time.minute() ;
                inQ << time.second() ;
                inQ << 0x9C ;
            }
                break ;
            case 'D' : {
                QDateTime UTC( QDateTime::currentDateTimeUtc() ) ;
                QDateTime local( UTC.toLocalTime() ) ;

                QByteArray date ;
                date.append( local.toString("dd MMM yy") ) ;
                inQ << 0x90 ;
                inQ << 'D' ;
                foreach( char t, date )
                    inQ << t ;
                inQ << 0x9C ;
            }
                break ;
            case 'd' : {
                QDate date( QDate::currentDate() ) ;
                inQ << 0x90 ;
                inQ << 'd' ;
                inQ << date.year() - 2000 ;
                inQ << date.month() ;
                inQ << date.day() ;
                inQ << 0x9C ;
            }
                break ;
            case 'L' :
                if ( dcsChars.size() == 4 ) {
//                    ledControl( dcsChars[ 1 ], dcsChars[ 2 ], dcsChars[ 3 ] ) ;
                    qDebug() << "Virtual LEDs:" << (uint8_t)dcsChars[ 1 ] << (uint8_t)dcsChars[ 2 ] << (uint8_t)dcsChars[ 3 ] ;
                } else
                    dcsChars << (char)c ;
                break ;
            case 'S' :
                getVirtualSwitches() ;
                break ;
            default :
                break ;
            }
            state = ptNone ;
        } else
            dcsChars << (char)c ;
        break ;

    case ptESC :
        if ( (char)c == '[' ) {
            state = ptESC2 ;
        } else
            state = ptNone ;
        break ;
    case ptESC2 :
        switch ( (char)c ) {
        case '2' :
            escChar = (char)c ;
            state = ptESC3 ;
            break ;
        case 0x1E ... 0x26 :
            state = ptNone ;
            break ;
        case 'H' :
            state = ptNone ;
            break ;
        default:
            state = ptNone ;
        }
        break ;
    case ptESC3 :
        switch ( (char)c ) {
        case 'J' :
            if ( escChar == '2' )
                emit clearScreen() ;
            state = ptNone ;
            break ;
        default:
            state = ptNone ;
        }
        break ;
    }
}

bool QmtxPicoTerm::eventFilter( QObject *obj, QEvent *event ) {
    if ( event->type() == QEvent::KeyPress ) {
        QKeyEvent * keyEvent = static_cast<QKeyEvent *>(event ) ;
        QString key = keyEvent->text() ;
        inQ.enqueue( key[ 0 ].toLatin1() ) ;
        return true ;
    } else
        return QObject::eventFilter(obj, event ) ;
}



