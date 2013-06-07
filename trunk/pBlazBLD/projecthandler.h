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
    void setFont(QFont font);

    void Init() ;
    void New() ;
    void Load(QString filename) ;
    void Save() ;
    void SaveAs() ;
    bool maybeSave() ;

    void addSourceFile( QString fileName ) ;
    void removeSourceFile(QString fileName) ;

    QStringList asmArguments() ;
    QStringList mrgArguments();
    QStringList bitArguments();

    QString fileName() ;

    QtTreePropertyBrowser * getVariantEditor( void ) ;

public Q_SLOTS:
    void setModified(QtProperty * prop, const QVariant &var ) ;

private:
    QString projectFileName ;
    VariantManager * variantManager ;
    VariantFactory * variantFactory ;
    QtTreePropertyBrowser * variantEditor ;
    bool modified ;

    QtVariantProperty * toolsItem, * editorItem, * asmItem, * mrgItem, * bitItem ;
    QtVariantProperty * asmOptions, * asmFiles, * asmSources ;
    QtVariantProperty * asmMEMfile, * asmSCRfile, * asmLSTfile ;
    QtVariantProperty * asmOptVerbose, * asmOptPBx ;

    QtVariantProperty * mrgOptions, * mrgFiles ;
    QtVariantProperty * mrgOptVerbose ;
    QtVariantProperty * mrgTPLfile ;

    QtVariantProperty * bitOptions, * bitFiles ;
    QtVariantProperty * bitOptVerbose ;
    QtVariantProperty * bitINfile, * bitOUTfile ;

    QtVariantProperty * fontItem ;
    QtVariantProperty * projectItem ;

    bool load_file() ;
    bool save_file() ;
    bool saveas() ;
    bool save() ;
} ;


#endif // PROJECTHANDLER_H
