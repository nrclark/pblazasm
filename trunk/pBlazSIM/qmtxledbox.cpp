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

#include "qmtxledbox.h"
#include "ui_qmtxledbox.h"

#include <QDebug>

QmtxLEDBox::QmtxLEDBox(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QmtxLEDBox)
{
    ui->setupUi( this ) ;

    ledsModel = new QStandardItemModel ;
    ledsModel->setHorizontalHeaderLabels( QStringList()
        << "LEDs"
    ) ;
    ledsModel->setVerticalHeaderLabels( QStringList()
        << "0" << "1" << "2" << "3"
        << "4" << "5" << "6" << "7"
    ) ;
    ui->tvLEDS->setModel( ledsModel ) ;
    for ( int row = 0 ; row < 8 ; row += 1 )
        ui->tvLEDS->setRowHeight( row, 16 ) ;
    ui->tvLEDS->setEditTriggers( QAbstractItemView::NoEditTriggers ) ;
    ui->tvLEDS->horizontalHeader()->setVisible( true ) ;
    ui->tvLEDS->verticalHeader()->setVisible( true ) ;
    columns = 0 ;
}

QmtxLEDBox::~QmtxLEDBox() {
    delete ui ;
}

void QmtxLEDBox::setFont( QFont font ) {
    ui->tvLEDS->setFont( font ) ;
}

bool QmtxLEDBox::addRack( int addr, int color ) {
    if ( ! addressMap.contains( addr ) ) {
        columns += 1 ;
        for ( int row = 0 ; row < 8 ; row += 1 ) {
            QStandardItem * item = new QStandardItem() ;
            item->setToolTip( "Double click to toggle state" ) ;
            ledsModel->setItem( row, columns - 1, item ) ;
        }
        LEDs * leds = new LEDs( color ) ;
        addressMap[ addr ] = leds ;
    }
    LEDs * leds = addressMap[ addr ] ;
    for ( int bits = 0 ; bits < 8 ; bits += 1 )
        leds->setItem( bits, ledsModel->item( bits, columns - 1 ) ) ;
    leds->update() ;
    itemMap[ columns - 1 ] = leds ;

    if ( addressMap.isEmpty() ) {
        ledsModel->setHorizontalHeaderLabels( QStringList() << "LEDs" ) ;
        return true ;
    }

    QStringList list ;
    foreach ( uint8_t addr, addressMap.keys() )
        list << QString( "0x%1" ).arg( addr, 2, 16, QChar('0') ) ;
    ledsModel->setHorizontalHeaderLabels( list ) ;
    for ( int col = 0 ; col < columns ; col += 1 )
       ui->tvLEDS->resizeColumnToContents( col ) ;
    return true ;
}

quint8 QmtxLEDBox::getValue( int addr ) {
    if ( ! addressMap.contains( addr ) )
        return 0 ;
    return addressMap[ addr ]->getValue() ;
}

void QmtxLEDBox::setValue( int addr, quint8 value ) {
    if ( ! addressMap.contains( addr ) )
        return ;
    addressMap[ addr ]->setValue( value ) ;
}

// toggle LEDs
void QmtxLEDBox::on_tvLEDS_doubleClicked( const QModelIndex &index ) {
    QStandardItem * item = ledsModel->itemFromIndex( index ) ;
    if ( item == NULL )
        return ;
    int row = item->row() ;
    int col = item->column() ;

    if ( ! itemMap.contains( col ) )
        return ;

    LEDs * leds = itemMap[ col ] ;
    int io = leds->getValue() ;
    leds->setValue( io ^ ( 1 << row ) ) ;
    leds->update() ;
}

// LEDs
LEDs::LEDs( int color ) {
    switch ( color ) {
    case 0:
        colorIcon = new QIcon(":/images/bullet_ball_glass_red.png");
        break ;
    case 1:
        colorIcon = new QIcon(":/images/bullet_ball_glass_green.png");
        break ;
    case 2:
        colorIcon = new QIcon(":/images/bullet_ball_glass_blue.png");
        break ;
    default:
        colorIcon = new QIcon(":/images/bullet_ball_glass_yellow.png");
    }
    blackIcon = new QIcon(":/images/bullet_ball_glass.png");

    rack = 0 ;
}

LEDs::~LEDs()
{
   delete colorIcon ;
   delete blackIcon ;
}

uint32_t LEDs::getValue(void ) {
    return rack ;
}

void LEDs::setValue( uint32_t value ) {
    rack = value ;
}

void LEDs::update( void ) {
    for ( int bits = 0 ; bits < 8 ; bits += 1 )
        if ( leds[ bits ] != NULL ) {
            if ( ( ( rack >> bits ) & 1 ) != 0 )
                leds[ bits ]->setIcon( *colorIcon ) ;
            else
                leds[ bits ]->setIcon( *blackIcon ) ;
        }
}

void LEDs::setItem ( uint32_t reg, QStandardItem * item ) {
    leds[ reg ] = item ;
}

