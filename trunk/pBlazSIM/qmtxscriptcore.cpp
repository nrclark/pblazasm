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

#include "qmtxscriptcore.h"
#include "mainwindow.h"

QmtxScriptCore::QmtxScriptCore(QObject *parent) :
    QObject(parent)
{
    w = NULL ;
}

void QmtxScriptCore::interrupt( void ) {
    Q_ASSERT( w != NULL ) ;
    return ( (MainWindow *)w )->scriptInterrupt() ;
}

void QmtxScriptCore::setIntVect( quint32 addr ) {
    Q_ASSERT( w != NULL ) ;
    ( (MainWindow *)w )->scriptSetIntVect( addr ) ;
}

void QmtxScriptCore::setHWBuild( quint8 value ) {
    Q_ASSERT( w != NULL ) ;
    ( (MainWindow *)w )->scriptSetHWBuild( value ) ;
}

void QmtxScriptCore::acknowledge( void ) {
    Q_ASSERT( w != NULL ) ;
    ( (MainWindow *)w )->scriptAcknowledge() ;
}
