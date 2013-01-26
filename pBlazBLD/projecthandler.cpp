#include "projecthandler.h"

#include <QDate>
#include <QLocale>
#include <QSizePolicy>

#include <QDebug>

QmtxProjectHandler::QmtxProjectHandler( QObject * parent ) {
    Q_UNUSED( parent ) ;

    variantManager = new VariantManager();
    variantFactory = new VariantFactory();

    variantEditor = new QtTreePropertyBrowser(  ) ;
    variantEditor->setAlternatingRowColors( true ) ;
    variantEditor->setFactoryForManager( variantManager, variantFactory ) ;
    variantEditor->setPropertiesWithoutValueMarked(true);
    variantEditor->setRootIsDecorated( true ) ;
    variantEditor->setResizeMode( QtTreePropertyBrowser::ResizeToContents ) ;

    QtVariantProperty * item, * projectItem ;
    QtBrowserItem * britem ;

    projectItem = variantManager->addProperty(QVariant::String, QLatin1String(" Project Name"));
    britem = variantEditor->addProperty( projectItem );
    variantEditor->setBackgroundColor( britem, QColor( 240, 240, 240, 255 ) );
    projectItem->setValue( "<untitled>" );


    editorItem = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),  QLatin1String(" Editor options"));
    britem = variantEditor->addProperty( editorItem );
    variantEditor->setBackgroundColor( britem, QColor( 250, 240, 240, 255 ) ) ;
    item = variantManager->addProperty( QVariant::Font, QLatin1String(" Font Property") ) ;
    editorItem->addSubProperty( item ) ;
    item = variantManager->addProperty(QVariant::SizePolicy, QLatin1String(" SizePolicy Property"));
    editorItem->addSubProperty( item ) ;


    asmItem = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), QLatin1String(" pBlazASM options"));
    britem = variantEditor->addProperty( asmItem );
    variantEditor->setBackgroundColor( britem, QColor( 240, 250, 240, 255 ) ) ;

    asmOptions = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), QLatin1String(" Options"));
    asmItem->addSubProperty( asmOptions ) ;

    item = variantManager->addProperty(QVariant::Bool, QLatin1String(" Verbose"));
    asmOptions->addSubProperty( item ) ;

    item = variantManager->addProperty(QtVariantPropertyManager::enumTypeId(), QLatin1String(" Picoblaze version"));
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
    item->setValue( "<defualt>" ) ;
    item->setAttribute( "filter", "Data files (*.SCR)" ) ;
    asmOptions->addSubProperty( item ) ;

    item = variantManager->addProperty(VariantManager::filePathTypeId(), "List (*.lst) file");
    item->setValue( "<default>" ) ;
    item->setAttribute( "filter", "List files (*.LST)" ) ;
    asmOptions->addSubProperty( item ) ;

    asmSources = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), QLatin1String(" Sources"));
    asmItem->addSubProperty( asmSources ) ;


    mrgItem = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), QLatin1String(" pBlazMRG options"));
    britem = variantEditor->addProperty( mrgItem );
    variantEditor->setBackgroundColor( britem, QColor( 240, 240, 250, 255 ) ) ;

    mrgOptions = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), QLatin1String(" Options"));
    mrgItem->addSubProperty( mrgOptions ) ;

    item = variantManager->addProperty(QVariant::Bool, QLatin1String(" Verbose"));
    mrgOptions->addSubProperty( item ) ;

    mrgSources = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), QLatin1String(" Sources"));
    mrgItem->addSubProperty( mrgSources ) ;

    item = variantManager->addProperty(VariantManager::filePathTypeId(), "Template File");
    item->setValue( "template.vhd" ) ;
    item->setAttribute( "filter", "Source files (*.vhd *.vhdl)" ) ;
    mrgSources->addSubProperty( item ) ;


    bitItem = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), QLatin1String(" pBlazBIT options"));
    britem = variantEditor->addProperty( bitItem );
    variantEditor->setBackgroundColor( britem, QColor( 240, 240, 200, 255 ) ) ;

    bitOptions = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), QLatin1String(" Options"));
    bitItem->addSubProperty( bitOptions ) ;

    item = variantManager->addProperty(QVariant::Bool, QLatin1String(" Verbose"));
    bitOptions->addSubProperty( item ) ;

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

//    item = variantManager->addProperty(QVariant::Int, QString::number(i++) + QLatin1String(" Int Property"));
//    item->setValue(20);
//    item->setAttribute(QLatin1String("minimum"), 0);
//    item->setAttribute(QLatin1String("maximum"), 100);
//    item->setAttribute(QLatin1String("singleStep"), 10);
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::Int, QString::number(i++) + QLatin1String(" Int Property (ReadOnly)"));
//    item->setValue(20);
//    item->setAttribute(QLatin1String("minimum"), 0);
//    item->setAttribute(QLatin1String("maximum"), 100);
//    item->setAttribute(QLatin1String("singleStep"), 10);
//    item->setAttribute(QLatin1String("readOnly"), true);
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::Double, QString::number(i++) + QLatin1String(" Double Property"));
//    item->setValue(1.2345);
//    item->setAttribute(QLatin1String("singleStep"), 0.1);
//    item->setAttribute(QLatin1String("decimals"), 3);
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::Double, QString::number(i++) + QLatin1String(" Double Property (ReadOnly)"));
//    item->setValue(1.23456);
//    item->setAttribute(QLatin1String("singleStep"), 0.1);
//    item->setAttribute(QLatin1String("decimals"), 5);
//    item->setAttribute(QLatin1String("readOnly"), true);
//    topItem->addSubProperty(item);


//    item = variantManager->addProperty(QVariant::String, QString::number(i++) + QLatin1String(" String Property"));
//    item->setValue("Value");
//    topItem->addSubProperty(item);

////    item = variantManager->addProperty(QVariant::StringList, QString::number(i++) + QLatin1String(" Stringlist Property"));
////    item->setValue(QStringList() << "Value");
////    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::String, QString::number(i++) + QLatin1String(" String Property (Password)"));
//    item->setAttribute(QLatin1String("echoMode"), QLineEdit::Password);
//    item->setValue("Password");
//    topItem->addSubProperty(item);

//    // Readonly String Property
//    item = variantManager->addProperty(QVariant::String, QString::number(i++) + QLatin1String(" String Property (ReadOnly)"));
//    item->setAttribute(QLatin1String("readOnly"), true);
//    item->setValue("readonly text");
//    topItem->addSubProperty(item);


//    item = variantManager->addProperty(QVariant::Date, QString::number(i++) + QLatin1String(" Date Property"));
//    item->setValue(QDate::currentDate().addDays(2));
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::Time, QString::number(i++) + QLatin1String(" Time Property"));
//    item->setValue(QTime::currentTime());
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::DateTime, QString::number(i++) + QLatin1String(" DateTime Property"));
//    item->setValue(QDateTime::currentDateTime());
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::KeySequence, QString::number(i++) + QLatin1String(" KeySequence Property"));
//    item->setValue(QKeySequence(Qt::ControlModifier | Qt::Key_Q));
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::Char, QString::number(i++) + QLatin1String(" Char Property"));
//    item->setValue(QChar(386));
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::Locale, QString::number(i++) + QLatin1String(" Locale Property"));
//    item->setValue(QLocale(QLocale::Polish, QLocale::Poland));
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::Point, QString::number(i++) + QLatin1String(" Point Property"));
//    item->setValue(QPoint(10, 10));
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::PointF, QString::number(i++) + QLatin1String(" PointF Property"));
//    item->setValue(QPointF(1.2345, -1.23451));
//    item->setAttribute(QLatin1String("decimals"), 3);
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::Size, QString::number(i++) + QLatin1String(" Size Property"));
//    item->setValue(QSize(20, 20));
//    item->setAttribute(QLatin1String("minimum"), QSize(10, 10));
//    item->setAttribute(QLatin1String("maximum"), QSize(30, 30));
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::SizeF, QString::number(i++) + QLatin1String(" SizeF Property"));
//    item->setValue(QSizeF(1.2345, 1.2345));
//    item->setAttribute(QLatin1String("decimals"), 3);
//    item->setAttribute(QLatin1String("minimum"), QSizeF(0.12, 0.34));
//    item->setAttribute(QLatin1String("maximum"), QSizeF(20.56, 20.78));
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::Rect, QString::number(i++) + QLatin1String(" Rect Property"));
//    item->setValue(QRect(10, 10, 20, 20));
//    topItem->addSubProperty(item);
//    item->setAttribute(QLatin1String("constraint"), QRect(0, 0, 50, 50));

//    item = variantManager->addProperty(QVariant::RectF, QString::number(i++) + QLatin1String(" RectF Property"));
//    item->setValue(QRectF(1.2345, 1.2345, 1.2345, 1.2345));
//    topItem->addSubProperty(item);
//    item->setAttribute(QLatin1String("constraint"), QRectF(0, 0, 50, 50));
//    item->setAttribute(QLatin1String("decimals"), 3);

//    item = variantManager->addProperty(QtVariantPropertyManager::enumTypeId(),
//                    QString::number(i++) + QLatin1String(" Enum Property"));
//    QStringList enumNames;
//    enumNames << "Enum0" << "Enum1" << "Enum2";
//    item->setAttribute(QLatin1String("enumNames"), enumNames);
//    item->setValue(1);
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QtVariantPropertyManager::flagTypeId(),
//                    QString::number(i++) + QLatin1String(" Flag Property"));
//    QStringList flagNames;
//    flagNames << "Flag0" << "Flag1" << "Flag2";
//    item->setAttribute(QLatin1String("flagNames"), flagNames);
//    item->setValue(5);
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::SizePolicy, QString::number(i++) + QLatin1String(" SizePolicy Property"));
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::Font, QString::number(i++) + QLatin1String(" Font Property"));
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::Cursor, QString::number(i++) + QLatin1String(" Cursor Property"));
//    topItem->addSubProperty(item);

//    item = variantManager->addProperty(QVariant::Color, QString::number(i++) + QLatin1String(" Color Property"));
//    topItem->addSubProperty(item);
}

QmtxProjectHandler::~QmtxProjectHandler() {
    delete variantManager;
    delete variantFactory;
}

void QmtxProjectHandler::addSourceFile( QString fileName ) {
    QtVariantProperty * item = variantManager->addProperty(VariantManager::filePathTypeId(), "Source File");
    item->setValue( fileName ) ;
    item->setAttribute( "filter", "Source files (*.psm *.psh)" ) ;
    asmSources->addSubProperty( item ) ;
}

void QmtxProjectHandler::removeSourceFile( QString fileName ) {
//    QtVariantProperty * item = variantManager->findProperty(VariantManager::filePathTypeId(), "Source File");
//    asmSources->removeSubProperty( item ) ;
    QList<QtProperty *> properties = asmSources->subProperties() ;
    QList<QtProperty *>::iterator i ;
    for ( i = properties.begin() ; i != properties.end() ; ++i ) {
        if ( (*i)->valueText() == fileName )
            asmSources->removeSubProperty( *i ) ;
    }
}

QtTreePropertyBrowser *QmtxProjectHandler::getVariantEditor() {
    return variantEditor ;
}


