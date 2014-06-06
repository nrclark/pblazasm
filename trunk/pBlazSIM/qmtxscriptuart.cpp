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

#include "qmtxscriptuart.h"
#include "mainwindow.h"

QmtxScriptUART::QmtxScriptUART(QObject *parent) :
    QObject(parent)
{
    w = NULL ;
}

quint8 QmtxScriptUART::getStatus ( void ) {
    Q_ASSERT( w != NULL ) ;
    return ( (MainWindow *)w )->getUARTstatus() ;
}

quint8 QmtxScriptUART::getData ( void ) {
    Q_ASSERT( w != NULL ) ;
    return ( (MainWindow *)w )->getUARTdata() ;
}

void QmtxScriptUART::setData ( quint8 value ) {
    Q_ASSERT( w != NULL ) ;
    ((MainWindow *)w)->setUARTdata( value ) ;
}
