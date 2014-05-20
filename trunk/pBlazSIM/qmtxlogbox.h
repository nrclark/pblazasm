// $Author: mediatronix@gmail.com $ $Date: 2014-04-13 11:38:29 +0200 (zo, 13 apr 2014) $ $Revision: 108 $

#ifndef QMXTLOGBOX_H
#define QMXTLOGBOX_H

#include <QPlainTextEdit>

void logBoxMessageOutput( QtMsgType type, const QMessageLogContext &context, const QString &msg ) ;

class QmtxLogBox : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit QmtxLogBox(QWidget *parent = 0);
    ~QmtxLogBox() ;
    
signals:
    
public slots:
    
private:
};

#endif // QMXTLOGBOX_H
