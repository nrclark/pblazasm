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

    Q_INVOKABLE bool addRack( int column, int addr, int color ) ;
    Q_INVOKABLE quint8 getValue( int addr ) ;
    Q_INVOKABLE void setValue(int addr, quint8 value ) ;

private slots:
    void on_tvLEDS_doubleClicked(const QModelIndex &index);

private:
    Ui::QmtxLEDBox * ui ;

    QStandardItemModel * ledsModel ;
    QMap< uint8_t, LEDs * > addressMap ;
    QMap< int, LEDs * > itemMap ;

} ;

#endif // QMTXLEDBOX_H
