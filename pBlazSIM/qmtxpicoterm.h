#ifndef QMTXPICOTERM_H
#define QMTXPICOTERM_H

#include <QObject>
#include <QQueue>
#include <QKeyEvent>
#include <QDebug>
#include <QDateTime>

#include <stdint.h>

enum ptState { ptNone, ptDCS, ptDCS2, ptESC, ptESC2, ptESC3 } ;

class QmtxPicoTerm : public QObject
{
    Q_OBJECT
public:
    explicit QmtxPicoTerm(QObject *parent = 0);

    uint32_t getChar( void ) {
        return inQ.dequeue() ;
    }

    void putChar( char c ) {
        inQ.enqueue( c ) ;
    }

    bool isEmpty( void ) {
        return inQ.isEmpty() ;
    }

signals:
    void sendChar( uint32_t c ) ;
    void showChar( uint32_t c ) ;

    void clearScreen( void ) ;
    void dcsLog( QList<uint32_t> l ) ;

    void ledControl( int r, int g, int b ) ;
    void getVirtualSwitches( void ) ;

public slots:
    void receivedChar( uint32_t c ) ;

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    enum ptState state ;
    QQueue<uint32_t> inQ ;
    QList<char> dcsChars ;
    char escChar ;

} ;

#endif // QMTXPICOTERM_H
