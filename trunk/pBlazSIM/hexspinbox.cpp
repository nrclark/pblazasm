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

#include "hexspinbox.h"

HexSpinBox::HexSpinBox( QWidget * parent ) :
    QSpinBox(parent)
{
    setRange( 0, 255 ) ;
    validator = new QRegExpValidator(QRegExp("[0-9A-Fa-f]{1,8}"), this);
    setAlignment( Qt::AlignCenter ) ;
}

HexSpinBox::~HexSpinBox() {
    delete validator ;
}

QValidator::State HexSpinBox::validate( QString &text, int &pos ) const {
    return validator->validate( text, pos ) ;
}

QString HexSpinBox::textFromValue( int value ) const {
    return QString( "%1" ).arg( value, 2, 16, QChar('0') ).toUpper() ;
}

int HexSpinBox::valueFromText(const QString &text) const {
    bool ok ;
    return text.toInt( &ok, 16 ) ;
}
