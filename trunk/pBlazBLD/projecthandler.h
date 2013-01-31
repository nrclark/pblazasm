#ifndef PROJECTHANDLER_H
#define PROJECTHANDLER_H

#include <QObject>

#include <QSizePolicy>
#include <QXmlStreamReader>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QFileDialog>

#include "variantmanager.h"
#include "variantfactory.h"

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"


class QmtxProjectHandler {
public:
    explicit QmtxProjectHandler( QObject *parent = 0 ) ;
    virtual ~QmtxProjectHandler() ;

    bool isModified();

    void Init();
    void New() ;
    void Load() ;
    void Save() ;
    bool maybeSave() ;

    void addSourceFile( QString fileName ) ;
    void removeSourceFile(QString fileName) ;

    QStringList asmArguments() ;

    QtTreePropertyBrowser * getVariantEditor( void ) ;
    QFont getFont() ;

public Q_SLOTS:
    void setModified(QtProperty * prop, const QVariant &var ) ;

private:
    QString projectFileName ;
    VariantManager * variantManager ;
    VariantFactory * variantFactory ;
    QtTreePropertyBrowser * variantEditor ;
    QFont * fixedFont ;
    bool modified ;

    QtVariantProperty * editorItem, * asmItem, * mrgItem, * bitItem ;
    QtVariantProperty * asmOptions, * asmSources ;
    QtVariantProperty * asmOptVerbose, * asmOptPBx ;

    QtVariantProperty * mrgOptions, * mrgSources ;
    QtVariantProperty * mrgOptVerbose ;

    QtVariantProperty * bitOptions, * bitSources ;
    QtVariantProperty * bitOptVerbose ;

    QtVariantProperty * fontItem ;
    QtVariantProperty * projectItem ;

    bool load_file();
    bool save_file();
    bool saveas();
    bool save() ;
} ;


#endif // PROJECTHANDLER_H
