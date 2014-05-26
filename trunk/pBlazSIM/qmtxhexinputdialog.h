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

#ifndef QMTXHEXINPUTDIALOG_H
#define QMTXHEXINPUTDIALOG_H

#include <QDialog>

namespace Ui {
    class QmtxHexInputDialog;
}

class QmtxHexInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QmtxHexInputDialog(QWidget *parent = 0);
    ~QmtxHexInputDialog();

    void setLabelText( QString s ) ;
    void setValue( quint32 value ) ;
    quint32 value( void ) ;

private:
    Ui::QmtxHexInputDialog *ui;
};

#endif // QMTXHEXINPUTDIALOG_H
