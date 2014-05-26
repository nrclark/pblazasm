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

#ifndef QMTXLEDBOX_H
#define QMTXLEDBOX_H

#include <QWidget>
#include <QStandardItemModel>
#include <QMap>

#include <stdint.h>

class LEDs : public QObject {
    Q_OBJECT
public:
    LEDs( int color ) ;
   ~LEDs() ;

    uint32_t getValue ( void ) ;
    void setValue ( uint32_t value ) ;
    void update( void ) ;

    void setItem ( uint32_t reg, QStandardItem * item ) ;

private:
    QStandardItem * leds[ 8 ] ;
    uint32_t rack ;

    QIcon * colorIcon ;
    QIcon * blackIcon ;
} ;

namespace Ui {
    class QmtxLEDBox ;
}

class QmtxLEDBox : public QWidget
{
    Q_OBJECT

public:
    explicit QmtxLEDBox(QWidget *parent = 0);
    ~QmtxLEDBox() ;

    void setFont( QFont font ) ;

    Q_INVOKABLE bool addRack( int addr, int color ) ;
    Q_INVOKABLE quint8 getValue( int addr ) ;
    Q_INVOKABLE void setValue( int addr, quint8 value ) ;

private slots:
    void on_tvLEDS_doubleClicked( const QModelIndex &index);

private:
    Ui::QmtxLEDBox * ui ;

    QStandardItemModel * ledsModel ;
    QMap< uint8_t, LEDs * > addressMap ;
    QMap< int, LEDs * > itemMap ;

    int columns ;
} ;

#endif // QMTXLEDBOX_H
