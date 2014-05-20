#include "qmtxscriptuart.h"

#include "mainwindow.h"

QmtxScriptUART::QmtxScriptUART(QObject *parent) :
    QObject(parent)
{
}

quint8 QmtxScriptUART::getStatus ( void ) {
    Q_ASSERT( w != NULL ) ;
    return ( (MainWindow *)w )->getUARTstatus() ;
}

quint8 QmtxScriptUART::getData ( void ) {
    Q_ASSERT( w != NULL ) ;
    return ( (MainWindow *)w )->getUARTdata() ;
}

void QmtxScriptUART::setData ( quint8 value ) {
    Q_ASSERT( w != NULL ) ;
    ((MainWindow *)w)->setUARTdata( value ) ;
}
