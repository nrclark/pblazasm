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

#ifndef QMTXSCRIPTUART_H
#define QMTXSCRIPTUART_H

#include <QObject>

// a link between script and terminal

class QmtxScriptUART : public QObject
{
    Q_OBJECT
public:
    explicit QmtxScriptUART(QObject *parent = 0);

    Q_INVOKABLE quint8 getStatus( void ) ;
    Q_INVOKABLE quint8 getData ( void ) ;
    Q_INVOKABLE void setData (quint8 value ) ;

    void * w ;

signals:

public slots:

} ;

#endif // QMTXSCRIPTUART_H
