#ifndef QMTXSCRIPTCORE_H
#define QMTXSCRIPTCORE_H

#include <QObject>

class QmtxScriptCore : public QObject
{
    Q_OBJECT
public:
    explicit QmtxScriptCore(QObject *parent = 0);

    Q_INVOKABLE void interrupt( void ) ;
    Q_INVOKABLE void setIntVect( quint32 addr ) ;
    Q_INVOKABLE void setHWBuild( quint8 value ) ;

    void * w ;

signals:

public slots:

} ;

#endif // QMTXSCRIPTCORE_H
