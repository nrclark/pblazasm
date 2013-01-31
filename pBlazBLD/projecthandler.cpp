#include <QDebug>

#include "projecthandler.h"
#include "qxmlsettings.h"

QmtxProjectHandler::QmtxProjectHandler( QObject * parent ) {
    Q_UNUSED( parent ) ;

    variantManager = new VariantManager() ;
    variantFactory = new VariantFactory() ;
    variantEditor = new QtTreePropertyBrowser(  ) ;
    fixedFont = new QFont( "Consolas [Monaco]", 9 ) ;

//    QObject::connect( (QtVariantPropertyManager *)variantManager, SIGNAL( valueChanged(QtProperty *, const QVariant &) ),
//                      (QObject *)this, SLOT( setModified(QtProperty *, const QVariant &) ) ) ;

    Init();
}

void QmtxProjectHandler::Init() {
    modified = false ;
    variantManager->clear() ;

    //variantManager->valueChanged();

    variantEditor->setFont( *fixedFont ) ;
    variantEditor->setAlternatingRowColors( true ) ;
    variantEditor->setFactoryForManager( (QtVariantPropertyManager *)variantManager, variantFactory ) ;
    variantEditor->setPropertiesWithoutValueMarked(true);
    variantEditor->setRootIsDecorated( true ) ;
    variantEditor->setResizeMode( QtTreePropertyBrowser::ResizeToContents ) ;

    QtVariantProperty * item ;
    QtBrowserItem * britem ;

    projectItem = variantManager->addProperty(QVariant::String, QLatin1String(" Project Name"));
    britem = variantEditor->addProperty( projectItem );
    variantEditor->setBackgroundColor( britem, QColor( 240, 240, 240, 255 ) );
    projectItem->setValue( "<untitled>" );


    editorItem = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),  QLatin1String(" Editor options"));
    britem = variantEditor->addProperty( editorItem ) ;
    variantEditor->setBackgroundColor( britem, QColor( 250, 240, 240, 255 ) ) ;

    fontItem = variantManager->addProperty( QVariant::Font, QLatin1String(" Font Property") ) ;
    variantManager->setValue( fontItem, *fixedFont ) ;
    editorItem->addSubProperty( fontItem ) ;
    fontItem = variantManager->addProperty(QVariant::SizePolicy, QLatin1String(" SizePolicy Property" ) ) ;
    editorItem->addSubProperty( fontItem ) ;
    variantEditor->setExpanded( britem, false ) ;

    // pBlazASM
    asmItem = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), QLatin1String(" pBlazASM options"));
    britem = variantEditor->addProperty( asmItem );
    variantEditor->setBackgroundColor( britem, QColor( 240, 250, 240, 255 ) ) ;

    asmOptions = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), QLatin1String(" Options"));
    asmItem->addSubProperty( asmOptions ) ;

    asmOptVerbose = variantManager->addProperty(QVariant::Bool, QLatin1String(" Verbose"));
    asmOptions->addSubProperty( asmOptVerbose ) ;

    asmOptPBx = variantManager->addProperty(QtVariantPropertyManager::enumTypeId(), QLatin1String(" Picoblaze version"));
    QStringList picoNames ;
    picoNames << "Picoblaze-3" << "Picoblaze-6" ;
    item->setAttribute( QLatin1String( "enumNames" ), picoNames ) ;
    item->setValue( 1 ) ;
    asmOptions->addSubProperty( item ) ;

    item = variantManager->addProperty(VariantManager::filePathTypeId(), "Code (*.mrg) File");
    item->setValue( "<default>" ) ;
    item->setAttribute( "filter", "Code files (*.MRG)" ) ;
    asmOptions->addSubProperty( item ) ;

    item = variantManager->addProperty(VariantManager::filePathTypeId(), "Data (*.scr) File");
    item->setValue( "<default>" ) ;
    item->setAttribute( "filter", "Data files (*.SCR)" ) ;
    asmOptions->addSubProperty( item ) ;

    item = variantManager->addProperty(VariantManager::filePathTypeId(), "List (*.lst) file");
    item->setValue( "<default>" ) ;
    item->setAttribute( "filter", "List files (*.LST)" ) ;
    asmOptions->addSubProperty( item ) ;

    asmSources = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), QLatin1String(" Sources"));
    asmItem->addSubProperty( asmSources ) ;

    // pBlazMRG
    mrgItem = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), QLatin1String(" pBlazMRG options"));
    britem = variantEditor->addProperty( mrgItem );
    variantEditor->setBackgroundColor( britem, QColor( 240, 240, 250, 255 ) ) ;

    mrgOptions = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), QLatin1String(" Options"));
    mrgItem->addSubProperty( mrgOptions ) ;

    mrgOptVerbose = variantManager->addProperty(QVariant::Bool, QLatin1String(" Verbose"));
    mrgOptions->addSubProperty( mrgOptVerbose ) ;

    mrgSources = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), QLatin1String(" Sources"));
    mrgItem->addSubProperty( mrgSources ) ;

    item = variantManager->addProperty(VariantManager::filePathTypeId(), "Template File");
    item->setValue( "template.vhd" ) ;
    item->setAttribute( "filter", "Source files (*.vhd *.vhdl)" ) ;
    mrgSources->addSubProperty( item ) ;

    // pBlazBIT
    bitItem = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), QLatin1String(" pBlazBIT options"));
    britem = variantEditor->addProperty( bitItem );
    variantEditor->setBackgroundColor( britem, QColor( 240, 240, 200, 255 ) ) ;

    bitOptions = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), QLatin1String(" Options"));
    bitItem->addSubProperty( bitOptions ) ;

    bitOptVerbose = variantManager->addProperty(QVariant::Bool, QLatin1String(" Verbose"));
    bitOptions->addSubProperty( bitOptVerbose ) ;

    bitSources = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), QLatin1String(" Options"));
    bitItem->addSubProperty( bitSources ) ;

    item = variantManager->addProperty(VariantManager::filePathTypeId(), "Input (*.bit) File");
    item->setValue( "" ) ;
    item->setAttribute( "filter", "Config files (*.bit *.bin)" ) ;
    bitSources->addSubProperty( item ) ;

    item = variantManager->addProperty(VariantManager::filePathTypeId(), "Output (*.bit) File");
    item->setValue( "" ) ;
    item->setAttribute( "filter", "Config files (*.bit *.bin)" ) ;
    bitSources->addSubProperty( item ) ;
}

QmtxProjectHandler::~QmtxProjectHandler() {
    delete variantManager;
    delete variantFactory;
}

void QmtxProjectHandler::New() {
    Init();
}

void QmtxProjectHandler::Load() {
    if ( maybeSave() ) {
        projectFileName = QFileDialog::getOpenFileName(
            0, "Open Project File", ".", "pBlazBLD project files (*.mtx);;All files (*.*)" ) ;
        if ( projectFileName.isNull() )
            return ;

        if ( ! load_file() ) {
            QMessageBox mb ;
            mb.setStandardButtons( QMessageBox::Ok ) ;
            mb.setInformativeText( projectFileName );
            mb.setText( "Could not load project file:" ) ;
            mb.exec() ;
            return ;
        }
    }
}

void QmtxProjectHandler::Save() {
    saveas();
}

void QmtxProjectHandler::addSourceFile( QString filename ) {
    if ( filename.isEmpty() )
        return ;

    QList<QtProperty *> properties = asmSources->subProperties() ;
    for ( QList<QtProperty *>::iterator i = properties.begin() ; i != properties.end() ; ++i )
        if ( (*i)->valueText() == filename )
            return ;

    int count = asmSources->subProperties().count() ;
    QtVariantProperty * item = variantManager->addProperty(VariantManager::filePathTypeId(), QString( "source-%1" ).arg( count ) ) ;
    item->setValue( filename ) ;
    item->setAttribute( "filter", "Source files (*.psm *.psh)" ) ;
    asmSources->addSubProperty( item ) ;
}

void QmtxProjectHandler::removeSourceFile( QString fileName ) {
    QList<QtProperty *> properties = asmSources->subProperties() ;

    for ( QList<QtProperty *>::iterator i = properties.begin() ; i != properties.end() ; ++i )
        if ( (*i)->valueText() == fileName )
            asmSources->removeSubProperty( *i ) ;
}

QStringList QmtxProjectHandler::asmArguments() {
    QStringList args ;

    args << ( asmOptPBx->value() == 1 ? "-6" : "-3" ) ;

    if ( asmOptVerbose->value().toBool() )
        args << "-v " ;

    args << "x" ;

    int count = asmSources->subProperties().count() ;

    return args ;
}

QtTreePropertyBrowser * QmtxProjectHandler::getVariantEditor() {
    return variantEditor ;
}

QFont QmtxProjectHandler::getFont() {
    return *fixedFont ;
}

void QmtxProjectHandler::setModified( QtProperty *prop, const QVariant &var ) {
    Q_UNUSED( prop ) ;
    Q_UNUSED( var ) ;
    modified = true ;
    qDebug() << "modified" ;
}

bool QmtxProjectHandler::maybeSave() {
    if ( isModified() ) {
        int ret = QMessageBox::warning( 0, "pBlazBLD",
             "The project has been modified.\nDo you want to save your changes?",
             QMessageBox::Yes | QMessageBox::No,
             QMessageBox::Cancel | QMessageBox::Escape
        ) ;
        if ( ret == QMessageBox::Yes )
            return save() ;
        else if ( ret == QMessageBox::Cancel )
            return false ;
    }
    return true ;
}

bool QmtxProjectHandler::save() {
    if ( projectFileName.isEmpty() )
        return saveas() ;
    else
        return save_file() ;
}

bool QmtxProjectHandler::saveas() {
    projectFileName = QFileDialog::getSaveFileName( 0 ) ;
    if ( projectFileName.isEmpty() )
        return false ;
    return save_file() ;
}

bool QmtxProjectHandler::load_file() {
    const QSettings::Format XmlFormat =
            QSettings::registerFormat( "mtx", readXmlFile, writeXmlFile ) ;

    QSettings settings( projectFileName, XmlFormat ) ;
//    if ( ! settings )
//            return false ;

    Init() ;
    asmOptVerbose->setValue( settings.value("pBlazASM/options/verbose").toBool() ) ;
    mrgOptVerbose->setValue( settings.value("pBlazMRG/options/verbose").toBool() ) ;
    bitOptVerbose->setValue( settings.value("pBlazBIT/options/verbose").toBool() ) ;

    settings.beginGroup( "pBlazASM" ) ;
        settings.beginGroup( "sources" ) ;
        int size = settings.value( "size" ).toInt() ;
        for ( int i = 0 ; i < size ; i += 1 ) {
            QString index = QString( "source-%1" ).arg( i ) ;
            QString filename = settings.value( index ).toString() ;
            QtVariantProperty * item = variantManager->addProperty(VariantManager::filePathTypeId(), index ) ;
            item->setValue( filename ) ;
            item->setAttribute( "filter", "Source files (*.psm *.psh)" ) ;
            asmSources->addSubProperty( item ) ;
        }
        settings.endGroup() ;
    settings.endGroup() ;

    return true ;
}

bool QmtxProjectHandler::save_file() {
    const QSettings::Format XmlFormat =
            QSettings::registerFormat( "mtx", readXmlFile, writeXmlFile ) ;

    QSettings settings( projectFileName, XmlFormat ) ;
//    if ( ! settings )
//            return false ;

    settings.beginGroup( "pBlazASM" ) ;
        settings.beginGroup( "options" ) ;
            settings.setValue( "verbose", true ) ;
            settings.setValue( "core", "pb3" ) ;
        settings.endGroup() ;

        settings.beginGroup( "sources" ) ;
        int count = asmSources->subProperties().count() ;

        settings.setValue( "size", count ) ;
        for ( int i = 0 ; i < count ; i += 1 ) {
            QString index = QString( "source-%1" ).arg( i ) ;
            settings.setValue( index, asmSources->subProperties()[i]->valueText() ) ;
        }
        settings.endGroup() ;
    settings.endGroup() ;

    settings.beginGroup( "pBlazMRG" ) ;
        settings.beginGroup( "options" ) ;
            settings.setValue( "verbose", true ) ;
        settings.endGroup() ;
        settings.beginGroup( "template" ) ;
            settings.setValue( "template", "template.vhdl" ) ;
        settings.endGroup() ;
    settings.endGroup() ;

    settings.beginGroup( "pBlazBIT" ) ;
        settings.beginGroup( "options" ) ;
            settings.setValue( "verbose", true ) ;
        settings.endGroup() ;
        settings.beginGroup( "bitfiles" ) ;
            settings.setValue( "infile", "infile.bit" ) ;
            settings.setValue( "outfile", "outfile.bit" ) ;
        settings.endGroup() ;
    settings.endGroup() ;

    return true ;
}


bool QmtxProjectHandler::isModified() {
    return modified ;
}


