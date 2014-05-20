#ifndef QMTXSCRIPTUART_H
#define QMTXSCRIPTUART_H

#include <QObject>

// a link between script and terminal

class QmtxScriptUART : public QObject
{
    Q_OBJECT
public:
    explicit QmtxScriptUART(QObject *parent = 0);

    Q_INVOKABLE quint8 getStatus( void ) ;
    Q_INVOKABLE quint8 getData ( void ) ;
    Q_INVOKABLE void setData (quint8 value ) ;

    void * w ;

signals:

public slots:

} ;

#endif // QMTXSCRIPTUART_H
