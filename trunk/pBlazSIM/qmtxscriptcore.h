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

#ifndef QMTXSCRIPTCORE_H
#define QMTXSCRIPTCORE_H

#include <QObject>

class QmtxScriptCore : public QObject
{
    Q_OBJECT
public:
    explicit QmtxScriptCore(QObject *parent = 0);

    Q_INVOKABLE void interrupt( void ) ;
    Q_INVOKABLE void setIntVect( quint32 addr ) ;
    Q_INVOKABLE void setHWBuild( quint8 value ) ;

    void acknowledge( void ) ;

    void * w ;

signals:

public slots:

} ;

#endif // QMTXSCRIPTCORE_H
