#include <QDebug>

#include "settingshandler.h"
#include "qxmlsettings.h"

QmtxSettingsHandler::QmtxSettingsHandler( QObject * parent ) {
    Q_UNUSED( parent ) ;

    variantManager = new VariantManager() ;
    variantFactory = new VariantFactory() ;
    variantEditor = new QtTreePropertyBrowser(  ) ;

    fixedFont = new QFont( "Consolas", 9 ) ;

//    QObject::connect( (QtVariantPropertyManager *)variantManager, SIGNAL( valueChanged(QtProperty *, const QVariant &) ),
//                      (QObject *)this, SLOT( setModified(QtProperty *, const QVariant &) ) ) ;

    Init() ;
    Read() ;
}

void QmtxSettingsHandler::Init() {
    QtBrowserItem * britem ;

    variantManager->clear() ;

    // variantManager->valueChanged();

    variantEditor->setFont( * fixedFont ) ;
    variantEditor->setAlternatingRowColors( false ) ;
    variantEditor->setFactoryForManager( (QtVariantPropertyManager *)variantManager, variantFactory ) ;
    variantEditor->setPropertiesWithoutValueMarked(true);
    variantEditor->setRootIsDecorated( true ) ;
    variantEditor->setResizeMode( QtTreePropertyBrowser::ResizeToContents ) ;

    toolsItem = variantManager->addProperty(VariantManager::groupTypeId(),  QLatin1String(" Tools"));
    britem = variantEditor->addProperty( toolsItem ) ;
    variantEditor->setBackgroundColor( britem, QColor( 250, 240, 255, 255 ) ) ;

    toolsASMfile = variantManager->addProperty(VariantManager::filePathTypeId(), QLatin1String("pBlazASM file path"));
    toolsASMfile->setValue( "./pBlazASM.exe" ) ;
    toolsASMfile->setAttribute( "filter", "EXE files (*.exe);;All files (*.*)" ) ;
    toolsItem->addSubProperty( toolsASMfile ) ;

    toolsMRGfile = variantManager->addProperty(VariantManager::filePathTypeId(), QLatin1String("pBlazMEM file path"));
    toolsMRGfile->setValue( "./pBlazMEM.exe" ) ;
    toolsMRGfile->setAttribute( "filter", "EXE files (*.exe);;All files (*.*)" ) ;
    toolsItem->addSubProperty( toolsMRGfile ) ;

    toolsBITfile = variantManager->addProperty(VariantManager::filePathTypeId(), QLatin1String("pBlazBIT file path"));
    toolsBITfile->setValue( "./pBlazBIT.exe" ) ;
    toolsBITfile->setAttribute( "filter", "EXE files (*.exe);;All files (*.*)" ) ;
    toolsItem->addSubProperty( toolsBITfile ) ;


    editorItem = variantManager->addProperty(VariantManager::groupTypeId(),  QLatin1String(" Editor options"));
    britem = variantEditor->addProperty( editorItem ) ;
    variantEditor->setBackgroundColor( britem, QColor( 250, 240, 240, 255 ) ) ;

    fontItem = variantManager->addProperty( QVariant::Font, QLatin1String(" Font Property") ) ;
    variantManager->setValue( fontItem, * fixedFont ) ;
    editorItem->addSubProperty( fontItem ) ;
    fontItem = variantManager->addProperty(QVariant::SizePolicy, QLatin1String(" SizePolicy Property" ) ) ;
    editorItem->addSubProperty( fontItem ) ;
    variantEditor->setExpanded( britem, false ) ;
}

void QmtxSettingsHandler::Read() {
    QSettings settings( "Mediatronix", "pBlazBLD" ) ;

    fixedFont->fromString( settings.value( "font", "Consolas,9,-1,5,50,0,0,0,0,0" ).toString() ) ;
    setPBlazASM( settings.value( "pBlazASM", "./pBlazASM" ).toString() ) ;
    setPBlazMRG( settings.value( "pBlazMRG", "./pBlazMRG" ).toString() ) ;
    setPBlazBIT( settings.value( "pBlazBIT", "./pBlazBIT" ).toString() ) ;
}

void QmtxSettingsHandler::Write() {
    QSettings settings( "Mediatronix", "pBlazBLD" ) ;

    settings.setValue( "font", fixedFont->toString() ) ;
    settings.setValue( "pBlazASM", pBlazASM() ) ;
    settings.setValue( "pBlazMRG", pBlazMRG() ) ;
    settings.setValue( "pBlazBIT", pBlazBIT() ) ;
}

QString QmtxSettingsHandler::pBlazASM() {
    if ( toolsASMfile )
        return toolsASMfile->valueText() ;
    return QString() ;
}

QString QmtxSettingsHandler::pBlazMRG() {
    if ( toolsMRGfile )
        return toolsMRGfile->valueText() ;
    return QString() ;
}

QString QmtxSettingsHandler::pBlazBIT() {
    if ( toolsBITfile )
        return toolsBITfile->valueText() ;
    return QString() ;
}

void QmtxSettingsHandler::setPBlazASM(QString path) {
    if ( toolsASMfile )
        toolsASMfile->setValue( path ) ;
}

void QmtxSettingsHandler::setPBlazMRG(QString path) {
    if ( toolsMRGfile )
        toolsMRGfile->setValue( path ) ;
}

void QmtxSettingsHandler::setPBlazBIT(QString path) {
    if ( toolsBITfile )
        toolsBITfile->setValue( path ) ;
}

QmtxSettingsHandler::~QmtxSettingsHandler() {
    delete variantManager;
    delete variantFactory;
}

QtTreePropertyBrowser * QmtxSettingsHandler::getVariantEditor() {
    return variantEditor ;
}

QFont QmtxSettingsHandler::getFont() {
    return *fixedFont ;
}

