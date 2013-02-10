#ifndef SETTINGSHANDLER_H
#define SETTINGSHANDLER_H

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


class QmtxSettingsHandler {
public:
    explicit QmtxSettingsHandler( QObject * parent = 0 ) ;
    virtual ~QmtxSettingsHandler() ;

    void Init() ;
    void Read() ;
    void Write() ;

    QString pBlazASM() ;
    QString pBlazMRG() ;
    QString pBlazBIT() ;

    void setPBlazASM( QString path ) ;
    void setPBlazMRG( QString path ) ;
    void setPBlazBIT( QString path ) ;

    QtTreePropertyBrowser * getVariantEditor( void ) ;
    QFont getFont() ;

public Q_SLOTS:

private:
    QString projectFileName ;
    VariantManager * variantManager ;
    VariantFactory * variantFactory ;
    QtTreePropertyBrowser * variantEditor ;
    QFont * fixedFont ;

    QtVariantProperty * toolsItem, * editorItem, * fontItem;
    QtVariantProperty * toolsASMfile, * toolsMRGfile, * toolsBITfile ;
} ;


#endif // SETTINGSHANDLER_H
