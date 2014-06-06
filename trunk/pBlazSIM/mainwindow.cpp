/*
 *  Copyright � 2003..2014 : Henk van Kampen <henk@mediatronix.com>
 *
 *  This file is part of pBlazASM.
 *
 *  pBlazASM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pBlazASM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pBlazASM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qmtxhexinputdialog.h"

#include <QDebug>
#include <QScrollBar>

// implement the script function 'print'
QScriptValue qs_print( QScriptContext * sc, QScriptEngine * se ) {
    qDebug() << sc->argument( 0 ).toString() ;
    return QScriptValue( se, 0 ) ;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    static QString defaultScript(
        "// default script\n"
        "// emulates UART at 0x00..0x01 \n"
        "// and 4 LEDs at 0x04..0x07\n"
        "\n"
        "LEDs.addRack( 0x04, 0 ) ; \n"
        "LEDs.addRack( 0x05, 1 ) ; \n"
        "LEDs.addRack( 0x06, 2 ) ; \n"
        "LEDs.addRack( 0x07, 3 ) ; \n"
        "\n"
        "function setData( port, value ) { \n"
        "    switch ( port ) { \n"
        "    case 0 : \n"
        "        break ; \n"
        "    case 1 : \n"
        "        UART.setData( value ) ; \n"
        "        break ; \n"
        "    case 4 : case 5 : case 6 : case 7 : \n"
        "        LEDs.setValue( port, value ) ; \n"
        "        break ; \n"
        "    case 0xF0 : \n"
        "        index = value ; \n"
        "        break ; \n"
        "    default : \n"
        "        print( \"setData-> port: \" + port + \", value: \" + value ) ; \n"
        "    } \n"
        "} \n"
        "\n"
        "// IN emulation \n"
        "function getData( port ) { \n"
        "    switch ( port ) { \n"
        "    case 0 : \n"
        "        return UART.getStatus() ; \n"
        "    case 1 : \n"
        "        return UART.getData() ; \n"
        "    case 4 : case 5 : case 6 : case 7 : \n"
        "        return LEDs.getValue( port ) ; \n"
        "    case 0xF0 : \n"
        "        return SBox[ index ] ; \n"
        "    default : \n"
        "        print( \"getData-> port: \" + port ) ; \n"
        "    } \n"
        "} \n"
    ) ;

    ui->setupUi( this ) ;

    timerId = 0 ;

    this->setWindowTitle( "pBlazSIM " + QString(PBLAZSIM_VERSION_STR) + " (Qt::" + QString(QT_VERSION_STR) + ") - http://www.mediatronix.com" ) ;
    this->setWindowIcon( QIcon( ":/files/bug_red.ico" ) ) ;

    // font used for all data views
    QFont fixedFont( "Consolas", 9 ) ;
    QFontInfo info( fixedFont ) ;
    QStringList altFonts ;
    altFonts << "Consolas" << "Lucida Console" << "Menlo" << "Monaco" << "Bitstream Vera" << "OCR_B" << "Andale Mono" << "Courier New" ;
    while ( info.family() != fixedFont.family()  && ! altFonts.isEmpty() ) {
        fixedFont.setFamily( altFonts.takeFirst() ) ;
        info = QFontInfo( fixedFont ) ;
    }
    QFontMetrics metrics( fixedFont ) ;

    // log box
    qInstallMessageHandler( logBoxMessageOutput ) ;

    // terminal support
    ui->teTerminal->installEventFilter( &picoTerm ) ;
    ui->teTerminal->setFont( fixedFont ) ;

    QPalette p = ui->teTerminal->palette() ;
    p.setColor( QPalette::Base, Qt::white ) ;
    p.setColor( QPalette::Text, Qt::black ) ;
    ui->teTerminal->setPalette( p ) ;

    // script editor
    ui->teScript->setFont( fixedFont ) ;

    // twIO to show scratchpad
    ui->twIO->setCurrentIndex( 0 ) ;

    // tbIO to show LEDs
    ui->tbIO->setCurrentIndex( 0 ) ;

    // LCD numbers to zero
    ui->db7Segment->display( "0000" ) ;

    // icons used for Code view
    redIcon = new QIcon(":/images/bullet_ball_glass_red.png" ) ;
    blueIcon = new QIcon(":/images/bullet_ball_glass_blue.png" ) ;


    // setup codemodel for codeview
    codeModel = new QStandardItemModel ;
    ui->tvCode->setModel( codeModel ) ;
    newCode() ;
    ui->tvCode->setFont( fixedFont ) ;
    ui->tvCode->setEditTriggers( QAbstractItemView::NoEditTriggers ) ;
    ui->tvCode->setSelectionBehavior( QAbstractItemView::SelectRows ) ;
    ui->tvCode->setSelectionMode( QAbstractItemView::NoSelection ) ;


    // setup statemodel for stateview
    stateModel = new QStandardItemModel ;
    stateModel->insertColumns( 0, 6 ) ;
    stateModel->setHorizontalHeaderLabels( QStringList() << "state" << "PC" << "Zero" << "Carry" << "Bank" << "IE" << "" ) ;
    for ( int column = 1 ; column < 6 ; column += 1 ) {
        QStandardItem * item = new QStandardItem( QString("") ) ;
        item->setTextAlignment( Qt::AlignCenter ) ;
        switch ( column ) {
        case 1 :
            item->setToolTip( "Double click to show source line" ) ;
            break ;
        default :
            item->setToolTip( "Double click to toggle" ) ;
        }
        stateModel->setItem( 0, column, item ) ;
        pBlaze.setStateItem( column - 1, item ) ;
    }
    ui->tvState->setModel( stateModel ) ;
    ui->tvState->setFont( fixedFont ) ;
    for ( int col = 0 ; col < 6 ; col += 1 )
        ui->tvState->setColumnWidth( col, COL_WIDTH ) ;
    ui->tvState->setEditTriggers( QAbstractItemView::NoEditTriggers ) ;
    QScrollBar * stateVSB = ui->tvState->verticalScrollBar() ;
    ui->tvState->setMinimumWidth( 6 * COL_WIDTH + 6 + stateVSB->sizeHint().width() ) ;
    QHeaderView * stateHeader = ui->tvState->header() ;
    ui->tvState->setMaximumHeight( stateHeader->height() + metrics.lineSpacing() + 6 ) ;
    stateHeader->setDefaultAlignment( Qt::AlignCenter ) ;


    // setup registermodel for registerview
    registerModel = new QStandardItemModel ;
    registerModel->insertColumns( 0, 2 ) ;
    registerModel->setHorizontalHeaderLabels( QStringList() << "reg#" << "value" ) ;
    for ( int reg = 0 ; reg < 16 ; reg += 1 ) {
        QStandardItem * item = new QStandardItem(" s" + QString("%1").arg(reg,1,16,QChar('0')).toUpper() ) ;
        item->setTextAlignment( Qt::AlignCenter ) ;
        registerModel->setItem( reg, 0, item ) ;

        item = new QStandardItem( "" ) ;
        item->setTextAlignment( Qt::AlignCenter ) ;
        item->setToolTip( "Double click to change" ) ;
        pBlaze.setRegisterItem( reg, item ) ;
        registerModel->setItem( reg, 1, item ) ;
    }
    ui->tvRegisters->setModel( registerModel ) ;
    ui->tvRegisters->setFont(fixedFont ) ;
    for ( int col = 0 ; col < 2 ; col += 1 )
        ui->tvRegisters->setColumnWidth( col, COL_WIDTH ) ;
    QScrollBar * registersVSB = ui->tvRegisters->verticalScrollBar() ;
    ui->tvRegisters->setMinimumWidth( 2 * COL_WIDTH + 6 + registersVSB->sizeHint().width() ) ;
    ui->tvRegisters->setEditTriggers( QAbstractItemView::NoEditTriggers ) ;
    QHeaderView * registerHeader = ui->tvRegisters->header() ;
    ui->tvRegisters->setMaximumHeight( registerHeader->height() + 16 * ( metrics.lineSpacing() + 6 ) ) ;
    registerHeader->setDefaultAlignment( Qt::AlignCenter ) ;


    // setup stackmodel for stackview
    stackModel = new QStandardItemModel ;
    stackModel->insertColumns( 0, 5 ) ;
    stackModel->setHorizontalHeaderLabels( QStringList() << "stack" << "PC" << "Zero" << "Carry" << "Bank" << "" ) ;
    for ( int sp = 0 ; sp < 32 ; sp += 1 ) {
        QStandardItem * item = new QStandardItem( QString("%1").arg( sp, 2, 10 ) ) ;
        item->setTextAlignment( Qt::AlignCenter ) ;
        stackModel->setItem( sp, 0, item ) ;

        item = new QStandardItem( "" ) ;
        item->setTextAlignment( Qt::AlignCenter ) ;
        item->setToolTip( "Double click to show source line" ) ;
        stackModel->setItem( sp, 1, item ) ;
        pBlaze.setStackItem( sp, item ) ;

        item = new QStandardItem( "" ) ;
        item->setTextAlignment( Qt::AlignCenter ) ;
        stackModel->setItem( sp, 2, item ) ;

        item = new QStandardItem( "" ) ;
        item->setTextAlignment( Qt::AlignCenter ) ;
        stackModel->setItem( sp, 3, item ) ;

        item = new QStandardItem( "" ) ;
        item->setTextAlignment( Qt::AlignCenter ) ;
        stackModel->setItem( sp, 4, item ) ;

        item = new QStandardItem( "" ) ;
        item->setTextAlignment( Qt::AlignCenter ) ;
        stackModel->setItem( sp, 5, item ) ;
    }
    ui->tvStack->setModel( stackModel ) ;
    ui->tvStack->setFont( fixedFont ) ;
    for ( int col = 0 ; col < 5 ; col += 1 )
        ui->tvStack->setColumnWidth( col, COL_WIDTH ) ;
    ui->tvStack->setEditTriggers( QAbstractItemView::NoEditTriggers ) ;
    QScrollBar * stackVSB = ui->tvStack->verticalScrollBar() ;
    ui->tvStack->setMinimumWidth( 5 * COL_WIDTH + 6 + stackVSB->sizeHint().width() ) ;
    QHeaderView * stackHeader = ui->tvStack->header() ;
    ui->tvStack->setMaximumHeight( stackHeader->height() + 32 * ( metrics.lineSpacing() + 6 ) ) ;
    stackHeader->setDefaultAlignment( Qt::AlignCenter ) ;


    // setup scratchpadmodel for scratchpadview
    scratchpadModel = new QStandardItemModel ;
    scratchpadModel->insertColumns( 0,16 ) ;
    scratchpadModel->insertRows( 0,16 ) ;
    scratchpadModel->setHorizontalHeaderLabels( QStringList()
        << "-0" << "-1" << "-2" << "-3"
        << "-4" << "-5" << "-6" << "-7"
        << "-8" << "-9" << "-A" << "-B"
        << "-C" << "-D" << "-E" << "-F"
    ) ;
    scratchpadModel->setVerticalHeaderLabels( QStringList()
        << "0-" << "1-" << "2-" << "3-"
        << "4-" << "5-" << "6-" << "7-"
        << "8-" << "9-" << "A-" << "B-"
        << "C-" << "D-" << "E-" << "F-"
    ) ;
    for ( int row = 0 ; row < 16 ; row += 1 )
        for ( int col = 0 ; col < 16 ; col += 1 ) {
            QStandardItem * item = new QStandardItem( "" ) ;
            scratchpadModel->setItem( row, col, item ) ;

            item->setTextAlignment( Qt::AlignCenter ) ;
            item->setToolTip( "Double click to change" ) ;
            pBlaze.setScratchpadItem( row * 16 + col, item ) ;
        }
    ui->tvScratchpad->setModel( scratchpadModel ) ;
    ui->tvScratchpad->setFont( fixedFont ) ;
    for ( int row = 0 ; row < 16 ; row += 1 )
        ui->tvScratchpad->setRowHeight( row, metrics.lineSpacing() + 3 ) ;
    for ( int col = 0 ; col < 16 ; col += 1 )
        ui->tvScratchpad->setColumnWidth( col, 28 ) ;
    ui->tvScratchpad->setEditTriggers( QAbstractItemView::NoEditTriggers ) ;


    // filewatcher, sees .lst file change
    fileWatch = new QFileSystemWatcher( this ) ;
    connect( fileWatch, SIGNAL(fileChanged(const QString&)), this, SLOT(fileWatch_fileChanged(const QString)) ) ;
    mbLSTchanged.setStandardButtons( QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel ) ;


    // settings in Registry
    QSettings settings( QSettings::NativeFormat, QSettings::UserScope, "Mediatronix", "pBlazSIM" ) ;
    if ( settings.contains( "files/LST" ) ) {
        QString s = settings.value("files/LST").toString() ;
        if ( s != "" )
            fileWatch->addPath( settings.value("files/LST").toString() ) ;
        if ( fileWatch->files().size() > 0 )
            statusBar()->showMessage( tr("Used before: ") + fileWatch->files()[ 0 ] ) ;

        setWindowState( (Qt::WindowStates)settings.value("mainwindow/state").toInt() ) ;
        if ( isFullScreen() | isMaximized() )
            setWindowState( Qt::WindowMaximized ) ;
        else
            setGeometry( settings.value("mainwindow/geometry").toRect() ) ;

        QList<int> sizes ;
        sizes.append( settings.value("mainsplitter/size0").toInt() ) ;
        sizes.append( settings.value("mainsplitter/size1").toInt() ) ;
        if ( sizes.size() > 0 )
            ui->splHorz->setSizes( sizes ) ;
    }

    // script engine
    engine = new QScriptEngine() ;
//    debugger = new QScriptEngineDebugger() ;
//    debugger->attachTo( engine ) ;
    engine->setProcessEventsInterval( 16 ) ;

    scriptFile.setFileName( "" ) ;

    QScriptValue svLEDBox = engine->newQObject( ui->wgLEDBox ) ;
    engine->globalObject().setProperty( "LEDs", svLEDBox ) ;
    ui->wgLEDBox->setFont( fixedFont ) ;

    uart = new QmtxScriptUART ;
    uart->w = this ;
    QScriptValue svUART = engine->newQObject( uart ) ;
    engine->globalObject().setProperty( "UART", svUART ) ;

    scriptCore = new QmtxScriptCore ;
    scriptCore->w = this ;
    QScriptValue svCore = engine->newQObject( scriptCore ) ;
    engine->globalObject().setProperty( "Core", svCore ) ;
    pBlaze.scriptCore = scriptCore ;

    engine->evaluate( defaultScript ) ;
    ui->teScript->appendPlainText( defaultScript ) ;

    // instatiate a script function 'print'
    engine->globalObject().setProperty( "print", engine->newFunction( qs_print, 1 ) ) ;

    connect( engine, SIGNAL(signalHandlerException(QScriptValue)),
             this, SLOT(signalHandlerException(QScriptValue)) ) ;

    ui->tbScript->setEnabled( false ) ;
    ui->actionEvaluate->setEnabled(  false ) ;

    // lexer for ECMAscript source
    lexer = new SharedTools::QScriptHighlighter( ui->teScript->document() ) ;

    // some GUI stuff
    ui->actionRefresh->setEnabled( fileWatch->files().size() > 0 ) ;
    ui->splHorz->setStretchFactor( 0, 20 ) ;
    ui->splHorz->setStretchFactor( 1, 80 ) ;

    ui->splScript->setStretchFactor( 0, 80 ) ;
    ui->splScript->setStretchFactor( 1, 20 ) ;


    // UI state
    connect( &pBlaze, SIGNAL(updateUI(enum pbState)), this, SLOT(updateUI(enum pbState)) ) ;


    // picoterm
    connect( &picoTerm, SIGNAL(clearScreen()), this, SLOT(clearScreen()) ) ;
    connect( &picoTerm, SIGNAL(showChar(uint32_t)), this, SLOT(showChar(uint32_t)) ) ;
    connect( &picoTerm, SIGNAL(dcsLog(QList<uint32_t>)), this, SLOT(dcsLog(QList<uint32_t>)) ) ;
}

// destructor
MainWindow::~MainWindow() {
    if ( pBlaze.configured() )
        pBlaze.initialize() ;

    if ( engine->isEvaluating() )
        engine->abortEvaluation() ;

    delete lexer ;

    // settings in Registry
    QSettings settings( QSettings::NativeFormat, QSettings::UserScope, "Mediatronix", "pBlazSIM" ) ;
    if ( fileWatch->files().size() > 0 )
        settings.setValue( "files/LST",  fileWatch->files()[ 0 ] ) ;
    else
        settings.setValue( "files/LST", "" ) ;

    settings.beginGroup( "mainwindow" ) ;
    settings.setValue( "state", (int)windowState() ) ;
    settings.setValue( "geometry", geometry() ) ;
    settings.endGroup() ;

    settings.beginGroup( "mainsplitter" ) ;
    settings.setValue( "size0", ui->splHorz->sizes()[ 0 ] ) ;
    settings.setValue( "size1", ui->splHorz->sizes()[ 1 ] ) ;
    settings.endGroup() ;

    settings.sync() ;

    delete ui ;
}

void MainWindow::on_actionAbout_triggered() {
    QMessageBox::about( this, "pBlazSIM", windowTitle() +
        "\n\nThis program comes with ABSOLUTELY NO WARRANTY." +
        "\nThis is free software, and you are welcome to redistribute it" +
        "\nunder certain conditions. See <http://www.gnu.org/licenses/>"
    ) ;
}

void MainWindow::on_actionAbout_Qt_triggered() {
    QMessageBox::aboutQt( this, "pBlazSIM" ) ;
}

void MainWindow::updateUI( enum pbState state ) {
    switch ( state ) {
    case psNone :
        ui->actionStep->setEnabled( false ) ;
        ui->actionJump->setEnabled( false ) ;
        ui->actionLeave->setEnabled( false ) ;
        ui->actionRun->setEnabled( false ) ;
        ui->actionReset->setEnabled( false ) ;
        ui->actionStop->setEnabled( false ) ;
        pBlaze.updateAll() ;
        statusBar()->showMessage(tr("Unconfigured") ) ;
        selectLine( QItemSelectionModel::Select ) ;
        break ;

    case psConfigured :
        ui->actionStep->setEnabled( false ) ;
        ui->actionJump->setEnabled( false ) ;
        ui->actionLeave->setEnabled( false ) ;
        ui->actionRun->setEnabled( false ) ;
        ui->actionReset->setEnabled( false ) ;
        ui->actionStop->setEnabled( false ) ;
        pBlaze.updateAll() ;
        statusBar()->showMessage(tr("Configured") ) ;
        selectLine( QItemSelectionModel::Select ) ;
        break ;

    case psReady :
    case psBarrier :
        ui->actionStep->setEnabled( true ) ;
        ui->actionJump->setEnabled( true ) ;
        ui->actionLeave->setEnabled( pBlaze.canLeave() ) ;
        ui->actionRun->setEnabled( true ) ;
        ui->actionReset->setEnabled( true ) ;
        ui->actionStop->setEnabled( false ) ;
        pBlaze.updateAll() ;
        statusBar()->showMessage(tr("Ready") ) ;
        selectLine( QItemSelectionModel::Select ) ;
        break ;

    case psBreakpoint :
        ui->actionStep->setEnabled( true ) ;
        ui->actionJump->setEnabled( true ) ;
        ui->actionLeave->setEnabled( pBlaze.canLeave() ) ;
        ui->actionRun->setEnabled( true ) ;
        ui->actionReset->setEnabled( true ) ;
        ui->actionStop->setEnabled( false ) ;
        pBlaze.updateAll() ;
        statusBar()->showMessage(tr("Breakpoint" ), 2000 ) ;
        selectLine( QItemSelectionModel::Select ) ;
        break ;

    case psRunning :
        ui->actionStep->setEnabled( false ) ;
        ui->actionJump->setEnabled( false ) ;
        ui->actionLeave->setEnabled( false ) ;
        ui->actionRun->setEnabled( false ) ;
        ui->actionReset->setEnabled( false ) ;
        ui->actionStop->setEnabled( true ) ;
        pBlaze.updateAll() ;
        statusBar()->showMessage(tr("Running"), 2000 ) ;
        break ;

    case psError :
        ui->actionStep->setEnabled( false ) ;
        ui->actionJump->setEnabled( false ) ;
        ui->actionLeave->setEnabled( false ) ;
        ui->actionRun->setEnabled( false ) ;
        ui->actionReset->setEnabled( true ) ;
        ui->actionStop->setEnabled( false ) ;
        pBlaze.updateAll() ;
        statusBar()->showMessage( pBlaze.getError() ) ;
        selectLine( QItemSelectionModel::Select ) ;
    }
}

void MainWindow::setupIO( void ) {
    if ( ! pBlaze.configured() )
        return ;

    // All emulated IO is scripted
    pBlaze.setIODevice( this, 0x00, 0xFF, new SCRIPT() ) ;
}

void MainWindow::closeEvent( QCloseEvent *event ) {
    event->accept() ;
}

void MainWindow::newCode( void ) {
    codeModel->clear() ;
    codeModel->insertColumns( 0, 5 ) ;
    codeModel->setHorizontalHeaderLabels( QStringList() << "line" << "bp" << "code" << "source" << "" ) ;
    ui->tvCode->setColumnWidth( 0, 40 ) ;
    ui->tvCode->setColumnWidth( 1, 25 ) ;
    ui->tvCode->setColumnWidth( 2, 90 ) ;
    ui->tvCode->setColumnWidth( 3, 300 ) ;
}

void MainWindow::on_actionNew_triggered() {
    pBlaze.setCore( QmtxPicoblaze::ctNone ) ;
    newCode() ;
}

// reload LST+SCR files
void MainWindow::on_actionRefresh_triggered() {
    if ( fileWatch->files().size() > 0 )
        loadLSTfile( fileWatch->files().first() ) ;
}

// select LST/LOG file
void MainWindow::on_action_Open_triggered() {
    QFileDialog dialog( this ) ;

    QStringList filters;
    filters << "pBlazASM list files (*.lst)"
            << "KCPSM6 log files (*.log)"
            << "Any files (*.*)";
    dialog.setNameFilters( filters ) ;

    if ( dialog.exec() ) {
        if ( fileWatch->files().count() > 0 )
            fileWatch->removePaths(fileWatch->files() ) ;

        fileWatch->addPaths( dialog.selectedFiles() ) ;
        loadLSTfile( fileWatch->files()[ 0 ] ) ;
    }
    ui->actionRefresh->setEnabled( fileWatch->files().size() > 0 ) ;
}

void MainWindow::fileWatch_fileChanged( const QString &path ) {
    fileWatch->removePath( path ) ;

    mbLSTchanged.setInformativeText( path ) ;

    QFile file( path ) ;
    if ( file.exists() ) {
        mbLSTchanged.setText( "LST file changed; reload?" ) ;
        switch ( mbLSTchanged.exec() ) {
        case QMessageBox::Yes :
            loadLSTfile( path ) ;
        case QMessageBox::Cancel :
            fileWatch->addPath( path ) ;
            break ;
        }
    } else {
        mbLSTchanged.setText( "File doesn't exist anymore, clear simulation?" ) ;
        switch ( mbLSTchanged.exec() ) {
        case QMessageBox::Yes :
            pBlaze.setCore( QmtxPicoblaze::ctNone ) ;
            newCode() ;
            break ;
        case QMessageBox::Cancel :
            fileWatch->addPath( path ) ;
            break ;
        }
    }
}

// load LST+SCR files, build code model/view
void MainWindow::loadLSTfile( QString filename ) {
    newCode() ;
    pBlaze.setCore( QmtxPicoblaze::ctNone ) ;

    QFile file( filename ) ;
    if ( !file.exists() ) {
        QMessageBox mb ;
        mb.setStandardButtons( QMessageBox::Ok ) ;
        mb.setInformativeText( filename ) ;
        mb.setText( "File doesn't exist anymore:" ) ;
        mb.exec() ;
        return ;
    }

    if ( !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox mb ;
        mb.setStandardButtons( QMessageBox::Ok ) ;
        mb.setInformativeText( filename ) ;
        mb.setText( "Could not open file:" ) ;
        mb.exec() ;
        return ;
    }

    // remove any filewatches
    while ( fileWatch->files().size() > 0 )
        fileWatch->removePath( fileWatch->files().last() ) ;
    fileWatch->addPath( filename ) ;
    ui->actionRefresh->setEnabled( fileWatch->files().size() > 0 ) ;

    // read .lst file and find code lines
    for ( int row = 0 ; ! file.atEnd() ; row += 1 ) {
         QStandardItem * item ;

         QByteArray line = file.readLine() ;
         QString str = line ;
         str.remove(QChar('\n') ) ;

         codeModel->insertRow( row ) ;

         item = new QStandardItem( QString("%1").arg(row+1) ) ;
         item->setSelectable( true ) ;
         item->setTextAlignment( Qt::AlignRight ) ;
         codeModel->setItem( row, 0, item ) ;

         if ( str.contains( QRegExp( "^PB3$" ) ) ) {
             pBlaze.setCore( QmtxPicoblaze::ctPB3 ) ;
         } else if ( str.contains( QRegExp( "^PB6$" ) ) ) {
             pBlaze.setCore( QmtxPicoblaze::ctPB6 ) ;
         } else if ( str.contains( QRegExp( "^PB7$" ) ) ) {
             pBlaze.setCore( QmtxPicoblaze::ctPB7 ) ;
         } else if ( str.contains( QRegExp( "^KCPSM6" ) ) ) {
             if ( row == 0 )
                 pBlaze.setCore( QmtxPicoblaze::ctPB6 ) ;
         } else if ( pBlaze.configured() && str.contains( QRegExp( "^[0-9A-F]{3,3} [0-9A-F]{5,5}" ) ) ) {
             item = new QStandardItem( *blueIcon, "" ) ;
             item->setSelectable( true ) ;

             bool error ;
             int address = str.left(3).toInt( &error, 16 ) ;
             int code = str.mid(4,5).toInt( &error, 16 ) ;

             item->setData( QVariant( address ), Qt::UserRole+1 ) ;
             pBlaze.setCodeItem( address, code, row + 1, item ) ;

             codeModel->setItem( row, 1, item ) ;
         } else if ( pBlaze.configured() && str.contains( QRegExp( "^ [0-9A-F]{3,3}  [0-9A-F]{5,5}" ) ) ) {
             item = new QStandardItem( *blueIcon, "" ) ;
             item->setSelectable( true ) ;

             bool error ;
             int address = str.mid(1,3).toInt( &error, 16 ) ;
             int code = str.mid(6,5).toInt( &error, 16 ) ;

             item->setData( QVariant( address ), Qt::UserRole+1 ) ;
             pBlaze.setCodeItem( address, code, row + 1, item ) ;

             codeModel->setItem( row, 1, item ) ;
         } else {
             item = new QStandardItem( "" ) ;
             item->setSelectable( true ) ;
             codeModel->setItem( row, 1, item ) ;
         }

         item = new QStandardItem( str.mid(0, 11) ) ;
         item->setSelectable( true ) ;
         item->setTextAlignment( Qt::AlignCenter ) ;
         codeModel->setItem( row, 2, item ) ;

         item = new QStandardItem( str.mid(11) ) ;
         item->setSelectable( true ) ;
         codeModel->setItem( row, 3, item ) ;
    }
    ui->tvCode->resizeColumnToContents( 3 ) ;
    file.close() ;

    QFileInfo fileInfo( fileWatch->files()[ 0 ] ) ;
    QString fp =  fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".scr" ;
    file.setFileName( fp ) ;

    if ( ! pBlaze.configured() )
        return ;

    // read scratchpad data (.scr) associated with .lst
    pBlaze.initScratchpad() ;
    if ( file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        for ( int addr = -1 ; addr < MAXMEM && !file.atEnd() ; ) {
            bool error ;
            QByteArray line = file.readLine() ;
            QString str = line ;
            str.remove(QChar('\n') ) ;

            if ( str[ 0 ] == '@' ) {
                addr = str.mid(1).toInt( &error, 16 ) & 0xFF ;
            } else {
                int value = str.toInt(&error, 16 ) ;
                pBlaze.setScratchpadValue( addr, value ) ;
                addr += 1 ;
            }
        }
        file.close() ;
    }
    setupIO() ;
    pBlaze.initialize() ;
    on_actionReset_triggered() ;
}

// exit application
void MainWindow::on_actionExit_triggered() {
    killTimer( timerId ) ;
    timerId = 0 ;
    close() ;
}

// simulation loop
void MainWindow::timerEvent( QTimerEvent * event ) {
    event->accept() ;
    if ( ! pBlaze.configured() )
        return ;

    updateUI( psRunning ) ;

    // run for a short time or until broken
    timer.start() ;
    while ( ! timer.hasExpired( 16 ) ) {
        for ( int i = 0 ; i < 10000 ; i +=1 ) {
            if ( pBlaze.onBreakpoint() || pBlaze.onBarrier() || ! pBlaze.step() ) {
                killTimer( timerId ) ;
                timerId = 0 ;

                if ( pBlaze.onBarrier() )
                    updateUI( psBarrier ) ;
                else if ( pBlaze.onBreakpoint() )
                    updateUI( psBreakpoint ) ;
                else
                    updateUI( psError ) ;

                pBlaze.resetBarrier() ;
                return ;
            }
        }
    }
}

// step into
void MainWindow::on_actionStep_triggered() {
    if ( ! pBlaze.configured() )
        return ;

    selectLine( QItemSelectionModel::Clear ) ;

    pBlaze.resetBarrier() ;
    if ( pBlaze.step() )
        updateUI( psReady ) ;
    else
        updateUI( psError ) ;
}

// step over
void MainWindow::on_actionJump_triggered() {
    if ( ! pBlaze.configured() )
        return ;

    selectLine( QItemSelectionModel::Clear ) ;

    pBlaze.setBarrier() ;
    timerId = startTimer( 16 ) ;
}

void MainWindow::on_actionLeave_triggered() {
    if ( ! pBlaze.configured() )
        return ;

    selectLine( QItemSelectionModel::Clear ) ;

    pBlaze.setCeiling() ; // uses barrier
    if ( pBlaze.onBreakpoint() ) {
        if ( ! pBlaze.step() ) {
            updateUI( psError ) ;
            return ;
        }
    }
    timerId = startTimer( 16 ) ;
}

// start simulation
void MainWindow::on_actionRun_triggered() {
    if ( ! pBlaze.configured() )
        return ;

    selectLine( QItemSelectionModel::Clear ) ;

    pBlaze.resetBarrier() ;
    if ( pBlaze.onBreakpoint() ) {
        if ( ! pBlaze.step() ) {
            updateUI( psError ) ;
            return ;
        }
    }
    timerId = startTimer( 16 ) ;
}

// reset simulation
void MainWindow::on_actionReset_triggered() {
    selectLine( QItemSelectionModel::Clear ) ;

    if ( timerId != 0 )
        killTimer( timerId ) ;
    timerId = 0 ;

    pBlaze.reset() ;
}

// stop simulation
void MainWindow::on_actionStop_triggered() {    
    selectLine( QItemSelectionModel::Clear ) ;

    if ( timerId != 0 )
        killTimer( timerId ) ;
    timerId = 0 ;

    pBlaze.stop() ;
}

// modify register
void MainWindow::on_tvRegisters_doubleClicked(const QModelIndex &index) {
    if ( ! pBlaze.configured() )
        return ;

    // item we clicked on
    QStandardItem * item = registerModel->itemFromIndex( index ) ;
    int row = item->row() ;

    QmtxHexInputDialog dialog ;
    dialog.setWindowTitle( "Registers" ) ;
    dialog.setWindowIcon( QIcon( ":/files/bug_red.ico" ) ) ;
    dialog.setLabelText( "New value of s" + QString("%1").arg(row,1,16,QChar('0')).toUpper() ) ;
    dialog.setValue( pBlaze.getRegisterValue( row ) ) ;
    if ( dialog.exec() )
        pBlaze.setRegisterValue( row, dialog.value() ) ;
}

// select/deselect helper
void MainWindow::selectLine( QItemSelectionModel::SelectionFlags option ) {
    if ( ! pBlaze.configured() )
        return ;

    QStandardItem * item = (QStandardItem *)pBlaze.getCurrentCodeItem() ;
    if ( item != NULL && item->index() != QModelIndex() ) {
        QItemSelectionModel * selection = ui->tvCode->selectionModel() ;
        if ( selection != NULL ) {
            selection->select( item->index(), option | QItemSelectionModel::Rows ) ;
            ui->tvCode->scrollTo( item->index(), QAbstractItemView::EnsureVisible ) ;
            ui->tvCode->setCurrentIndex( item->index() ) ;
        }
    }
}

void MainWindow::on_tvStack_doubleClicked(const QModelIndex &index ) {
    if ( ! pBlaze.configured() )
        return ;

    QStandardItem * item = stackModel->itemFromIndex( index ) ;
    int row = item->row() ;
    int col = item->column() ;

    if ( col == 1 ) {
        int retaddr = pBlaze.getStackPcValue( row ) ;
        item = (QStandardItem *)pBlaze.getCodeItem( retaddr ) ;
        if ( item != NULL ) {
//            QItemSelectionModel * selection = ui->tvCode->selectionModel() ;
//            if ( selection != NULL ) {
//                selection->select( item->index(), QItemSelectionModel::Select | QItemSelectionModel::Rows ) ;
                ui->tvCode->scrollTo( item->index(), QAbstractItemView::EnsureVisible ) ;
                ui->tvCode->setCurrentIndex( item->index() ) ;
                ui->tvCode->setFocus( Qt::MouseFocusReason ) ;
//            }
        }
    }
}

// modify scratchpad cell
void MainWindow::on_tvScratchpad_doubleClicked(const QModelIndex &index) {
    if ( ! pBlaze.configured() )
        return ;

    QStandardItem * item = scratchpadModel->itemFromIndex( index ) ;
    int row = item->row() ;
    int col = item->column() ;

    int cell = row * 16 + col ;

    QmtxHexInputDialog dialog ;
    dialog.setWindowTitle( "Scratchpad" ) ;
    dialog.setWindowIcon( QIcon( ":/files/bug_red.ico" ) ) ;
    dialog.setLabelText( "New value at 0x" + QString("%1").arg( cell,2,16,QChar('0') ).toUpper() ) ;
    dialog.setValue( pBlaze.getScratchpadValue( cell ) ) ;
    if ( dialog.exec() )
        pBlaze.setScratchpadValue( cell, dialog.value() ) ;
}

void MainWindow::on_tvState_doubleClicked( const QModelIndex &index ) {
    if ( ! pBlaze.configured() )
        return ;

    QStandardItem * item = stateModel->itemFromIndex( index ) ;
    int row = item->row() ;
    int col = item->column() ;

    if ( row == 0 ) {
        switch ( col ) {
        case 1 : {
            uint32_t pc = pBlaze.getPcValue() ;
            item = (QStandardItem *)pBlaze.getCodeItem( pc ) ;
            if ( item != NULL ) {
                ui->tvCode->scrollTo( item->index(), QAbstractItemView::EnsureVisible ) ;
                ui->tvCode->setCurrentIndex( item->index() ) ;
                ui->tvCode->setFocus( Qt::MouseFocusReason ) ;
            }
        }
            break ;
        case 2 :
            pBlaze.toggleZero() ;
            break ;
        case 3 :
            pBlaze.toggleCarry() ;
            break ;
        case 4 :
            pBlaze.toggleBank() ;
            break ;
        case 5 :
            pBlaze.toggleEnable() ;
            break ;
        default:
            break ;
        }
        pBlaze.updatePicoState() ;
    }
}

// toggle breakoint
void MainWindow::on_tvCode_doubleClicked(const QModelIndex &index) {
    if ( ! pBlaze.configured() )
        return ;

    // clicked somewhere in the row
    int row = codeModel->itemFromIndex(index)->row() ;
    // get the bullet cell
    QStandardItem * item = codeModel->item( row, 1 ) ;
    if ( item == NULL )
        return ;

    // get the associated address
    QVariant data = item->data( Qt::UserRole+1 ) ;
    if ( ! data.isValid() )
        return ;

    bool ok ;
    int addr = data.toInt( &ok ) ;

    // set or reset the breakpoint at that address
    if ( pBlaze.getBreakpoint( addr ) ) {
        item->setIcon( *blueIcon ) ;
        pBlaze.resetBreakpoint( addr ) ;
    } else {
        item->setIcon( *redIcon  ) ;
        pBlaze.setBreakpoint( addr ) ;
    }
}

void MainWindow::on_tvCode_clicked(const QModelIndex &index) {
    if ( ! pBlaze.configured() )
        return ;

    // clicked somewhere in the row
    int row = codeModel->itemFromIndex(index)->row() ;
    // get the bullit cell
    QStandardItem * item = codeModel->item( row, 1 ) ;
    if ( item == NULL )
        return ;

    // get the associated address
    QVariant data = item->data( Qt::UserRole+1 ) ;
    if ( ! data.isValid() )
        return ;

    bool ok ;
    int addr = data.toInt( &ok ) ;

    statusBar()->showMessage(
        QString("Instruction at: 0x%1 was executed: %2 times")
        .arg( addr, 3, 16, QChar('0') )
        .arg( pBlaze.getCodeCount( addr ) )
    ) ;
}

// remove all breakpoints
void MainWindow::on_actionRemove_triggered() {
    if ( ! pBlaze.configured() )
        return ;

    for ( int addr = 0 ; addr < MAXMEM ; addr += 1 ) {
        if ( pBlaze.getBreakpoint( addr ) ) {
            pBlaze.resetBreakpoint( addr ) ;
            QStandardItem * item = (QStandardItem *)pBlaze.getCodeItem( addr ) ;
            if ( item != NULL )
                item->setIcon( *blueIcon ) ;
        }
    }
}

// UART to terminal (QPlainTextView in crippled mode) support
uint32_t MainWindow::getUARTstatus( void ) {
    // only the data available bit
    if ( picoTerm.isEmpty() )
        return 0x00 ;
    else
        return 0x08 ;
}

uint32_t MainWindow::getUARTdata( void ) {
    // get the character
    return picoTerm.getChar() ;
}

void MainWindow::setUARTdata( uint32_t c ) {
    picoTerm.receivedChar( c ) ;
}

void MainWindow::showChar( uint32_t c ) {
    // put it on screen (skip new lines)
    ui->teTerminal->insertPlainText( QString( QChar(c) ) ) ;
//    if ( c > 127 )
//        qDebug() << c << "??" ;
    ui->teTerminal->ensureCursorVisible() ;
}

void MainWindow::clearScreen( void ) {
    ui->teTerminal->clear() ;
}

void MainWindow::dcsLog( QList<uint32_t> l ) {
    QString s ;
    foreach ( uint32_t c, l )
        s.append( QChar(c) ) ;
    statusBar()->showMessage( s, 2000 ) ;
}

void MainWindow::getVirtualSwitches( void ) {
    QList<char> list ;
    uint16_t switches = 0 ;
    switches |= ui->cbVS7->isChecked() ? 1 << 15 : 0 ;
    switches |= ui->cbVS6->isChecked() ? 1 << 14 : 0 ;
    switches |= ui->cbVS5->isChecked() ? 1 << 13 : 0 ;
    switches |= ui->cbVS4->isChecked() ? 1 << 12 : 0 ;
    switches |= ui->cbVS3->isChecked() ? 1 << 11 : 0 ;
    switches |= ui->cbVS2->isChecked() ? 1 << 10 : 0 ;
    switches |= ui->cbVS1->isChecked() ? 1 << 9 : 0 ;
    switches |= ui->cbVS0->isChecked() ? 1 << 8 : 0 ;

    switches |= ui->cbVS7_2->isChecked() ? 1 << 7 : 0 ;
    switches |= ui->cbVS6_2->isChecked() ? 1 << 6 : 0 ;
    switches |= ui->cbVS5_2->isChecked() ? 1 << 5 : 0 ;
    switches |= ui->cbVS4_2->isChecked() ? 1 << 4 : 0 ;
    switches |= ui->cbVS3_2->isChecked() ? 1 << 3 : 0 ;
    switches |= ui->cbVS2_2->isChecked() ? 1 << 2 : 0 ;
    switches |= ui->cbVS1_2->isChecked() ? 1 << 1 : 0 ;
    switches |= ui->cbVS0_2->isChecked() ? 1 << 0 : 0 ;
    list << 0x90 << 'S' << ( switches & 0xFF ) << ( switches >> 8 ) << 0x9C ;
    qDebug() << list ;
    foreach( char c, list )
        picoTerm.putChar( c ) ;
}

uint32_t MainWindow::getScriptValue( uint32_t address ) {
    QScriptValue global = engine->globalObject() ;

    QScriptValue getData = global.property( "getData" ) ;
    QScriptValue result = getData.call( QScriptValue(), QScriptValueList() << address ) ;
    if ( engine->hasUncaughtException() )
        qDebug() << "getScriptValue error:" << result.toString() << ", line#:" << engine->uncaughtExceptionLineNumber() ;
    return result.toNumber() ;
}

void MainWindow::setScriptValue( uint32_t address, uint32_t value ) {
    QScriptValue global = engine->globalObject() ;

    QScriptValue setData = global.property( "setData" ) ;
    QScriptValue error = setData.call( QScriptValue(), QScriptValueList() << address << value ) ;
    if ( engine->hasUncaughtException() )
        qDebug() << "setScriptValue error:" << error.toString() << ", line#:" << engine->uncaughtExceptionLineNumber() ;
}

void MainWindow::scriptInterrupt( void ) {
    if ( ! pBlaze.configured() )
        return ;
    pBlaze.setInterrupt( true ) ;
}

void MainWindow::scriptAcknowledge( void ) {
    QScriptValue global = engine->globalObject() ;

    QScriptValue ack = global.property( "acknowledge" ) ;
    QScriptValue error = ack.call( QScriptValue() ) ;
    if ( engine->hasUncaughtException() )
        qDebug() << "acknowledge error:" << error.toString() << ", line#:" << engine->uncaughtExceptionLineNumber() ;
}

void MainWindow::scriptSetIntVect( quint32 addr ) {
    if ( ! pBlaze.configured() )
        return ;
    pBlaze.setIntVect( addr ) ;
}

void MainWindow::scriptSetHWBuild( quint8 value ) {
    if ( ! pBlaze.configured() )
        return ;
    pBlaze.setHWBuild( value ) ;
}


void MainWindow::on_actionLoad_triggered() {
    QFileDialog dialog( this ) ;
    dialog.setWindowTitle( "Load IO script file" ) ;
    dialog.setAcceptMode( QFileDialog::AcceptOpen ) ;

    if ( ! scriptFile.fileName().isEmpty() ) {
        QFileInfo info( scriptFile ) ;
        dialog.setDirectory( info.absoluteDir() ) ;
    }

    QStringList filters;
    filters << "Script files files (*.js)"
            << "Any files (*.*)";
    dialog.setNameFilters( filters ) ;

    if ( ! dialog.exec() )
        return ;

    QStringList files ;
    files = dialog.selectedFiles() ;
    if ( files.isEmpty() )
        return ;

    scriptFile.setFileName( files.first() ) ;

    if ( ! scriptFile.open( QIODevice::ReadOnly ) )
        qDebug() << "can't open IO config script file:" << scriptFile.fileName() ;
    else {
        QTextStream stream( &scriptFile ) ;
        QString program = stream.readAll() ;
        scriptFile.close() ;
        ui->teScript->clear() ;
        ui->teScript->setPlainText( program ) ;
        engine->evaluate( program ) ;
    }
}

void MainWindow::on_actionSave_Script_triggered() {
    QFileDialog dialog( this ) ;
    dialog.setWindowTitle( "Save IO script file" ) ;
    dialog.setAcceptMode( QFileDialog::AcceptSave ) ;

    if ( ! scriptFile.fileName().isEmpty() ) {
        QFileInfo info( scriptFile ) ;
        dialog.setDirectory( info.absoluteDir() ) ;
    }

    QStringList filters ;
    filters << "Script files files (*.js)"
            << "Any files (*.*)";
    dialog.setNameFilters( filters ) ;

    if ( ! dialog.exec() )
        return ;

    QStringList files ;
    files = dialog.selectedFiles() ;
    if ( files.isEmpty() )
        return ;

    scriptFile.setFileName( files.first() ) ;

    if ( ! scriptFile.open( QIODevice::WriteOnly ) )
        qDebug() << "can't create IO config script file:" << scriptFile.fileName() ;
    else {
        QTextStream stream( &scriptFile ) ;
        QString program = ui->teScript->toPlainText() ;
        stream << program ;
        scriptFile.close() ;
        QSettings settings( QSettings::NativeFormat, QSettings::UserScope, "Mediatronix", "pBlazSIM" ) ;
        settings.setValue( "files/script", scriptFile.fileName() ) ;
    }
}

void MainWindow::on_actionEvaluate_triggered() {
    if ( engine->isEvaluating() )
        engine->abortEvaluation() ;
    QString program = ui->teScript->toPlainText() ;
    if ( engine->canEvaluate( program ) )
        engine->evaluate( program ) ;
    else
        qDebug() << "scriptEngine: can't evaluate script due to syntax errors" ;
    ui->actionEvaluate->setEnabled( scriptContext == NULL ) ;
}

void MainWindow::signalHandlerException( QScriptValue value ){
    qDebug() << "exception in script signal:" << value.toString() ;
}

void MainWindow::on_twIO_currentChanged(int index) {
    ui->tbScript->setEnabled( index == 1 ) ;
}

void MainWindow::on_teScript_textChanged() {
    ui->actionEvaluate->setEnabled( true ) ;
}
