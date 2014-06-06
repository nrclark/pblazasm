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

// $Author: mediatronix@gmail.com $ $Date: 2014-04-13 11:38:29 +0200 (zo, 13 apr 2014) $ $Revision: 108 $

#ifndef QMXTLOGBOX_H
#define QMXTLOGBOX_H

#include <QPlainTextEdit>

void logBoxMessageOutput( QtMsgType type, const QMessageLogContext &context, const QString &msg ) ;

class QmtxLogBox : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit QmtxLogBox(QWidget *parent = 0);
    ~QmtxLogBox() ;
} ;

#endif // QMXTLOGBOX_H
