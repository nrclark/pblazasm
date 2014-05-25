
#include "qmtxscriptcore.h"
#include "mainwindow.h"

QmtxScriptCore::QmtxScriptCore(QObject *parent) :
    QObject(parent)
{
}

void QmtxScriptCore::interrupt( void ) {
    Q_ASSERT( w != NULL ) ;
    return ( (MainWindow *)w )->scriptInterrupt() ;
}

void QmtxScriptCore::setIntVect( quint32 addr ) {
    Q_ASSERT( w != NULL ) ;
    ( (MainWindow *)w )->scriptSetIntVect( addr ) ;
}

void QmtxScriptCore::setHWBuild( quint8 value ) {
    Q_ASSERT( w != NULL ) ;
    ( (MainWindow *)w )->scriptSetHWBuild( value ) ;
}
