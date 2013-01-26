#ifndef PROJECTHANDLER_H
#define PROJECTHANDLER_H

#include <QXmlStreamReader>
#include <QFile>
#include <QFileInfo>

#include "variantmanager.h"
#include "variantfactory.h"

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"



class QmtxProjectHandler {
public:
    explicit QmtxProjectHandler( QObject *parent = 0 ) ;
    virtual ~QmtxProjectHandler() ;

    void addSourceFile( QString fileName ) ;
    void removeSourceFile(QString fileName) ;

    QtTreePropertyBrowser * getVariantEditor( void ) ;

private:
    QtVariantPropertyManager * variantManager ;
    QtVariantEditorFactory * variantFactory ;
    QtTreePropertyBrowser * variantEditor ;

    QtProperty * editorItem, * asmItem, * mrgItem, * bitItem ;
    QtProperty * asmOptions, * asmSources ;
    QtProperty * mrgOptions, * mrgSources ;
    QtProperty * bitOptions, * bitSources ;

} ;


#endif // PROJECTHANDLER_H
