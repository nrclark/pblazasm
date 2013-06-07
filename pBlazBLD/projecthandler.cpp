#include <QDebug>

#include "projecthandler.h"
#include "qxmlsettings.h"

QmtxProjectHandler::QmtxProjectHandler( QObject * parent ) {
    Q_UNUSED( parent ) ;

    variantManager = new VariantManager() ;
    variantFactory = new VariantFactory() ;
    variantEditor = new QtTreePropertyBrowser(  ) ;

//    QObject::connect( (QtVariantPropertyManager *)variantManager, SIGNAL( valueChanged(QtProperty *, const QVariant &) ),
//                      (QObject *)this, SLOT( setModified(QtProperty *, const QVariant &) ) ) ;

    Init();
}

void QmtxProjectHandler::Init() {
    QtBrowserItem * britem ;

    modified = false ;
    variantManager->clear() ;

    // variantManager->valueChanged();

    variantEditor->setAlternatingRowColors( false ) ;
    variantEditor->setFactoryForManager( (QtVariantPropertyManager *)variantManager, variantFactory ) ;
    variantEditor->setPropertiesWithoutValueMarked(true);
    variantEditor->setRootIsDecorated( true ) ;
    variantEditor->setResizeMode( QtTreePropertyBrowser::ResizeToContents ) ;

    projectItem = variantManager->addProperty(QVariant::String, QLatin1String(" Project Name"));
    britem = variantEditor->addProperty( projectItem );
    variantEditor->setBackgroundColor( britem, QColor( 240, 240, 240, 255 ) );
    projectItem->setValue( "<untitled>" ) ;

    // pBlazASM
    asmItem = variantManager->addProperty(VariantManager::groupTypeId(), QLatin1String(" pBlazASM options"));
    britem = variantEditor->addProperty( asmItem );
    variantEditor->setBackgroundColor( britem, QColor( 240, 250, 240, 255 ) ) ;

    asmOptions = variantManager->addProperty(VariantManager::groupTypeId(), QLatin1String(" Options"));
    asmItem->addSubProperty( asmOptions ) ;

    asmOptVerbose = variantManager->addProperty(QVariant::Bool, QLatin1String(" Verbose"));
    asmOptions->addSubProperty( asmOptVerbose ) ;

    asmOptPBx = variantManager->addProperty(VariantManager::enumTypeId(), QLatin1String(" Picoblaze version"));
    QStringList picoNames ;
    picoNames << "Picoblaze-3" << "Picoblaze-6" ;
    asmOptPBx->setAttribute( QLatin1String( "enumNames" ), picoNames ) ;
    asmOptPBx->setValue( 1 ) ;
    asmOptions->addSubProperty( asmOptPBx ) ;

    asmFiles = variantManager->addProperty(VariantManager::groupTypeId(), QLatin1String(" Files"));
    asmItem->addSubProperty( asmFiles ) ;

    asmMEMfile = variantManager->addProperty(VariantManager::filePathTypeId(), QLatin1String("Code (*.mem) file"));
    asmMEMfile->setValue( "<default>" ) ;
    asmMEMfile->setAttribute( "filter", "MEM files (*.mem);;All files (*.*)" ) ;
    asmFiles->addSubProperty( asmMEMfile ) ;
    asmSCRfile = variantManager->addProperty(VariantManager::filePathTypeId(), QLatin1String("Data (*.scr) file"));
    asmSCRfile->setValue( "<default>" ) ;
    asmSCRfile->setAttribute( "filter", "SCR files (*.scr);;All files (*.*)" ) ;
    asmFiles->addSubProperty( asmSCRfile ) ;
    asmLSTfile = variantManager->addProperty(VariantManager::filePathTypeId(), QLatin1String("Listing (*.lst) file"));
    asmLSTfile->setValue( "<default>" ) ;
    asmLSTfile->setAttribute( "filter", "LST files (*.lst);;All files (*.*)" ) ;
    asmFiles->addSubProperty( asmLSTfile ) ;

    asmSources = variantManager->addProperty(VariantManager::groupTypeId(), QLatin1String(" Sources"));
    asmItem->addSubProperty( asmSources ) ;

    // pBlazMRG
    mrgItem = variantManager->addProperty(VariantManager::groupTypeId(), QLatin1String(" pBlazMRG options"));
    britem = variantEditor->addProperty( mrgItem );
    variantEditor->setBackgroundColor( britem, QColor( 240, 240, 250, 255 ) ) ;

    mrgOptions = variantManager->addProperty(VariantManager::groupTypeId(), QLatin1String(" Options"));
    mrgItem->addSubProperty( mrgOptions ) ;

    mrgOptVerbose = variantManager->addProperty(QVariant::Bool, QLatin1String(" Verbose"));
    mrgOptions->addSubProperty( mrgOptVerbose ) ;

    mrgFiles = variantManager->addProperty(VariantManager::groupTypeId(), QLatin1String(" Files"));
    mrgItem->addSubProperty( mrgFiles ) ;

    mrgTPLfile = variantManager->addProperty(VariantManager::filePathTypeId(), "Template (*.tpl) file");
    mrgTPLfile->setValue( "<undefined>" ) ;
    mrgTPLfile->setAttribute( "filter", "Template files (*.tpl *.vhd);;All files (*.*)" ) ;
    mrgFiles->addSubProperty( mrgTPLfile ) ;

    // pBlazBIT
    bitItem = variantManager->addProperty(VariantManager::groupTypeId(), QLatin1String(" pBlazBIT options"));
    britem = variantEditor->addProperty( bitItem );
    variantEditor->setBackgroundColor( britem, QColor( 240, 240, 200, 255 ) ) ;

    bitOptions = variantManager->addProperty(VariantManager::groupTypeId(), QLatin1String(" Options"));
    bitItem->addSubProperty( bitOptions ) ;

    bitOptVerbose = variantManager->addProperty(QVariant::Bool, QLatin1String(" Verbose"));
    bitOptions->addSubProperty( bitOptVerbose ) ;

    bitFiles = variantManager->addProperty(VariantManager::groupTypeId(), QLatin1String(" Options"));
    bitItem->addSubProperty( bitFiles ) ;

    bitINfile = variantManager->addProperty(VariantManager::filePathTypeId(), "Input (*.bit) File");
    bitINfile->setValue( "<undefined>" ) ;
    bitINfile->setAttribute( "filter", "Config files (*.bit *.bin);;All files (*.*)" ) ;
    bitFiles->addSubProperty( bitINfile ) ;

    bitOUTfile = variantManager->addProperty(VariantManager::filePathTypeId(), "Output (*.bit) File");
    bitOUTfile->setValue( "<undefined>" ) ;
    bitOUTfile->setAttribute( "filter", "Config files (*.bit *.bin);;All files (*.*)" ) ;
    bitFiles->addSubProperty( bitOUTfile ) ;
}

QmtxProjectHandler::~QmtxProjectHandler() {
    delete variantManager;
    delete variantFactory;
}

void QmtxProjectHandler::New() {
    Init();
}

void QmtxProjectHandler::Load( QString filename ) {
    if ( maybeSave() ) {
        if ( filename.isEmpty() ) {
            projectFileName = QFileDialog::getOpenFileName(
                0, "Open Project File", ".", "pBlazBLD project files (*.mtx);;All files (*.*)" ) ;
            if ( projectFileName.isEmpty() )
                return ;
        } else {
            projectFileName = filename ;
        }

        if ( ! load_file() ) {
            QMessageBox mb ;
            mb.setStandardButtons( QMessageBox::Ok ) ;
            mb.setInformativeText( projectFileName );
            mb.setText( "? Could not load project file:" ) ;
            mb.exec() ;
            return ;
        }
    }
}

void QmtxProjectHandler::Save() {
    _save() ;
}

void QmtxProjectHandler::SaveAs() {
    _save_as() ;
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
        args << "-v" ;

    if ( ! asmLSTfile->value().toString().isEmpty() ) {
        if ( asmLSTfile->value().toString() == "<default>" )
            args << "-l" ;
        else
            args << "-l" << QDir::toNativeSeparators ( asmLSTfile->value().toString() ) ;
    }

    int count = asmSources->subProperties().count() ;
    for ( int i = 0 ; i < count ; i += 1 )
        args << QDir::toNativeSeparators ( asmSources->subProperties()[i]->valueText() ) ;

    return args ;
}

QStringList QmtxProjectHandler::mrgArguments() {
    QStringList args ;

    if ( mrgOptVerbose->value().toBool() )
        args << "-v" ;

    return args ;
}

QStringList QmtxProjectHandler::bitArguments() {
    QStringList args ;

    if ( bitOptVerbose->value().toBool() )
        args << "-v" ;

    return args ;
}

QString QmtxProjectHandler::fileName() {
    return projectFileName ;
}

QtTreePropertyBrowser * QmtxProjectHandler::getVariantEditor() {
    return variantEditor ;
}

void QmtxProjectHandler::setModified( QtProperty *prop, const QVariant &var ) {
    Q_UNUSED( prop ) ;
    Q_UNUSED( var ) ;
    modified = true ;
}

bool QmtxProjectHandler::maybeSave() {
    if ( isModified() ) {
        int ret = QMessageBox::warning( 0, "QtSDR",
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
    projectFileName = QFileDialog::getSaveFileName(
        0, "Open Project File", ".", "pBlazBLD project files (*.mtx);;All files (*.*)" ) ;

    if ( projectFileName.isEmpty() )
        return false ;
    return save_file() ;
}

bool QmtxProjectHandler::load_file() {
    if ( projectFileName.isEmpty() )
        return false ;

    const QSettings::Format XmlFormat =
            QSettings::registerFormat( "mtx", readXmlFile, writeXmlFile ) ;

    QSettings settings( projectFileName, XmlFormat ) ;
    int size ;

    Init() ;

    projectItem->setValue( projectFileName ) ;
    bitOptVerbose->setValue( settings.value("pBlazBIT/options/verbose").toBool() ) ;

    settings.beginGroup( "pBlazASM" ) ;
        asmOptVerbose->setValue( settings.value("options/verbose").toBool() ) ;
        asmMEMfile->setValue( settings.value( "files/MEM" ).toString() ) ;
        asmSCRfile->setValue( settings.value( "files/SCR" ).toString() ) ;
        asmLSTfile->setValue( settings.value( "files/LST" ).toString() ) ;

        settings.beginGroup( "sources" ) ;
        size = settings.value( "size" ).toInt() ;
        for ( int i = 0 ; i < size ; i += 1 ) {
            QString index = QString( "file-%1" ).arg( i ) ;
            QString filename = settings.value( index ).toString() ;
            QtVariantProperty * item = variantManager->addProperty(VariantManager::filePathTypeId(), index ) ;
            item->setValue( filename ) ;
            item->setAttribute( "filter", "Source files (*.psm *.psh);;All files (*.*)" ) ;
            asmSources->addSubProperty( item ) ;
        }
        settings.endGroup() ;
    settings.endGroup() ;

    settings.beginGroup( "pBlazMRG" ) ;
        mrgOptVerbose->setValue( settings.value("options/verbose").toBool() ) ;
        mrgTPLfile->setValue( settings.value( "files/LST" ).toString() ) ;
    settings.endGroup() ;

    settings.beginGroup( "pBlazBIT" ) ;
        bitOptVerbose->setValue( settings.value("options/verbose").toBool() ) ;
        bitINfile->setValue( settings.value( "files/IN" ).toString() ) ;
        bitOUTfile->setValue( settings.value( "files/OUT" ).toString() ) ;
    settings.endGroup() ;

    return true ;
}

bool QmtxProjectHandler::save_file() {
    if ( projectFileName.isEmpty() )
        return false ;

    const QSettings::Format XmlFormat =
            QSettings::registerFormat( "mtx", readXmlFile, writeXmlFile ) ;

    QSettings settings( projectFileName, XmlFormat ) ;
    int size = 0 ;

    settings.beginGroup( "pBlazASM" ) ;
        settings.setValue( "options/verbose", asmOptVerbose->value() ) ;
        settings.setValue( "options/core", asmOptPBx->value() ) ;
        settings.setValue( "files/MEM", asmMEMfile->valueText() ) ;
        settings.setValue( "files/SCR", asmSCRfile->valueText() ) ;
        settings.setValue( "files/LST", asmLSTfile->valueText() ) ;

        settings.beginGroup( "sources" ) ;
            size = asmSources->subProperties().count() ;
            settings.setValue( "size", size ) ;
            for ( int i = 0 ; i < size ; i += 1 ) {
                QString index = QString( "file-%1" ).arg( i ) ;
                settings.setValue( index, asmSources->subProperties()[i]->valueText() ) ;
            }
        settings.endGroup() ;
    settings.endGroup() ;

    settings.beginGroup( "pBlazMRG" ) ;
        settings.setValue( "options/verbose", mrgOptVerbose->value() ) ;
        settings.setValue( "files/TPL", mrgTPLfile->valueText() ) ;
    settings.endGroup() ;

    settings.beginGroup( "pBlazBIT" ) ;
        settings.setValue( "options/verbose", bitOptVerbose->value() ) ;
        settings.setValue( "files/IN", bitINfile->valueText() ) ;
        settings.setValue( "files/OUT", bitOUTfile->valueText() ) ;
    settings.endGroup() ;

    return true ;
}


bool QmtxProjectHandler::isModified() {
    return modified ;
}


void QmtxProjectHandler::setFont( QFont font ) {
    variantEditor->setFont( font ) ;
}


