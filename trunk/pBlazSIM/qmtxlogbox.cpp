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

#include "qmtxlogbox.h"

static QmtxLogBox * single = NULL ;
static bool logging = false ;

void logBoxMessageOutput( QtMsgType type, const QMessageLogContext &context, const QString &msg ) {
    if ( logging )
        return ;
    logging = true ;
    (void)type ;
    (void)context ;
    if ( single != NULL )
        single->appendPlainText( msg ) ;
    logging = false ;
}

QmtxLogBox::QmtxLogBox( QWidget * parent ) :
    QPlainTextEdit(parent)
{
    setFont( QFont( "Consolas", 8, QFont::Normal ) ) ;
    setReadOnly( true ) ;
    setMaximumBlockCount( 1024 ) ;
    setCenterOnScroll( true ) ;
    // only first
    if ( single == NULL )
        single = this ;
}

QmtxLogBox::~QmtxLogBox() {
    single = NULL ;
}


