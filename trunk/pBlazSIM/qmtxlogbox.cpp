// $Author: mediatronix@gmail.com $ $Date: 2014-04-13 11:38:29 +0200 (zo, 13 apr 2014) $ $Revision: 108 $

#include "qmtxlogbox.h"

static QmtxLogBox * single = NULL ;
static bool logging = false ;

void logBoxMessageOutput( QtMsgType type, const QMessageLogContext &context, const QString &msg ) {
    if ( logging )
        return ;
    logging = true ;
    (void)type ;
    (void)context ;
    if ( single != NULL )
        single->appendPlainText( msg ) ;
//        single->appendPlainText( msg + "  (" + context.function + ")" ) ;
    logging = false ;
}

QmtxLogBox::QmtxLogBox( QWidget * parent ) :
    QPlainTextEdit(parent)
{
    setFont( QFont( "Consolas", 8, QFont::Normal ) ) ;
    setReadOnly( true ) ;
    setMaximumBlockCount( 1024 ) ;
    setCenterOnScroll( true ) ;
    // only first
    if ( single == NULL )
        single = this ;
}

QmtxLogBox::~QmtxLogBox() {
    single = NULL ;
}


