/*
 *  Copyright © 2003..2012 : Henk van Kampen <henk@mediatronix.com>
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

#include "QtDebug"

bool KeyPressEater::eventFilter(QObject *obj, QEvent *event)
 {
     if ( event->type() == QEvent::KeyPress ) {
         QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
//         qDebug("Ate key press %d", keyEvent->key());
        QString key = keyEvent->text() ;
        w->UART_IN->enqueue( key[0].toAscii() );
         return true;
     } else {
         // standard event processing
         return QObject::eventFilter(obj, event);
     }
}

// constructor, build all supporting cores, models, devices
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QFont fixedFont( "Consolas", 9 ) ;

    eater = new KeyPressEater();
    eater->w = this ;
    ui->teTerminal->installEventFilter( eater ) ;
    ui->teTerminal->setFont(fixedFont);

    pBlaze = new Picoblaze();

    blueIcon = new QIcon(":/images/bullet_ball_glass_blue.png");
    redIcon = new QIcon(":/images/bullet_ball_glass_red.png");

    fileWatch = new QFileSystemWatcher( this ) ;

    codeModel = new QStandardItemModel ;
    codeModel->insertColumns(0,4);
    codeModel->setHorizontalHeaderLabels((QStringList() << "line" << "bp" << "code" << "source"));
    ui->tvCode->setModel( codeModel ) ;
    ui->tvCode->setFont(fixedFont);
    ui->tvCode->setColumnWidth(0, 45);
    ui->tvCode->setColumnWidth(1, 25);
    ui->tvCode->setColumnWidth(2, 100);
    ui->tvCode->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->tvCode->setSelectionBehavior( QAbstractItemView::SelectRows ) ;
    ui->tvCode->setSelectionMode( QAbstractItemView::NoSelection ) ;

    stateModel = new QStandardItemModel ;
    stateModel->insertColumns(0,2);
    stateModel->setHorizontalHeaderLabels((QStringList() << "state" << "value"));
    QStandardItem * item ;

    item = new QStandardItem(QString("pc"));
    stateModel->setItem( 0, 0, item);

    item = new QStandardItem(QString("sp"));
    stateModel->setItem( 1, 0, item);

    item = new QStandardItem(QString("zero"));
    stateModel->setItem( 2, 0, item);

    item = new QStandardItem(QString("carry"));
    stateModel->setItem( 3, 0, item);

    item = new QStandardItem(QString("enable"));
    stateModel->setItem( 4, 0, item);

    for ( int row = 0 ; row < 5 ; row += 1 ) {
        item = new QStandardItem(QString(""));
        stateModel->setItem( row, 1, item);
        pBlaze->setStateItem( row, item ) ;
    }

    ui->tvState->setModel( stateModel ) ;
    ui->tvState->setFont(fixedFont);
    for ( int row = 0 ; row < 16 ; row += 1 )
        ui->tvState->setRowHeight(row, 16 );
    for ( int col = 0 ; col < 2 ; col += 1 )
        ui->tvState->setColumnWidth(col, 50);
    ui->tvState->setEditTriggers(QAbstractItemView::NoEditTriggers);


    registerModel = new QStandardItemModel ;
    registerModel->insertColumns(0,2);
    registerModel->setHorizontalHeaderLabels((QStringList() << "registers" << "value"));
    for ( int reg = 0 ; reg < 16 ; reg += 1 ) {
        QStandardItem * item = new QStandardItem(" s"+QString("%1").arg(reg,1,16).toUpper());
        registerModel->setItem( reg, 0, item);

        item = new QStandardItem(QString("%1").arg(0,4,16).toUpper()) ;
        pBlaze->setRegisterItem( reg, item ) ;
        registerModel->setItem( reg, 1, item );
    }
    ui->tvRegisters->setModel( registerModel ) ;
    ui->tvRegisters->setFont(fixedFont);
    for ( int row = 0 ; row < 16 ; row += 1 )
        ui->tvRegisters->setRowHeight(row, 16 );
    for ( int col = 0 ; col < 2 ; col += 1 )
        ui->tvRegisters->setColumnWidth(col, 50);
    ui->tvRegisters->setEditTriggers(QAbstractItemView::NoEditTriggers);


    stackModel = new QStandardItemModel ;
    stackModel->insertColumns(0,4);
    stackModel->setHorizontalHeaderLabels((QStringList() << "stack" << "pc" << "zero" << "carry"));
    for ( int sp = 0 ; sp < 32 ; sp += 1 ) {
        QStandardItem * item = new QStandardItem(QString("%1").arg(sp,2,10));
        stackModel->setItem( sp, 0, item);

        item = new QStandardItem(QString("%1").arg(0, 5, 16));
        pBlaze->setStackItem( sp, item ) ;
        stackModel->setItem( sp, 1, item );
    }
    ui->tvStack->setModel( stackModel ) ;
    ui->tvStack->setFont(fixedFont);
    for ( int row = 0 ; row < 32 ; row += 1 )
        ui->tvStack->setRowHeight(row, 16 );
    for ( int col = 0 ; col < 4 ; col += 1 )
        ui->tvStack->setColumnWidth(col, 50);
    ui->tvStack->setEditTriggers(QAbstractItemView::NoEditTriggers);


    scratchpadModel = new QStandardItemModel ;
    scratchpadModel->insertColumns(0,16);
    scratchpadModel->insertRows(0,16);
    scratchpadModel->setHorizontalHeaderLabels( (QStringList()
        << "X0" << "X1" << "X2" << "X3"
        << "X4" << "X5" << "X6" << "X7"
        << "X8" << "X9" << "XA" << "XB"
        << "XC" << "XD" << "XE" << "XF"
    ));
    scratchpadModel->setVerticalHeaderLabels( (QStringList()
        << "0X" << "1X" << "2X" << "3X"
        << "4X" << "5X" << "6X" << "7X"
        << "8X" << "9X" << "AX" << "BX"
        << "CX" << "DX" << "EX" << "FX"
    ));
    for ( int row = 0 ; row < 16 ; row += 1 )
     for ( int col = 0 ; col < 16 ; col += 1 ) {
        QStandardItem * item = new QStandardItem(QString("%1").arg(0,2,16).toUpper());
        pBlaze->setScratchpadItem( row*16+col, item ) ;
        scratchpadModel->setItem( row, col, item );
     }
    ui->tvScratchpad->setModel( scratchpadModel ) ;
    ui->tvScratchpad->setFont(fixedFont);
    for ( int row = 0 ; row < 16 ; row += 1 )
        ui->tvScratchpad->setRowHeight(row, 16 );
    for ( int col = 0 ; col < 16 ; col += 1 )
        ui->tvScratchpad->setColumnWidth(col, 28);
    ui->tvScratchpad->setEditTriggers(QAbstractItemView::NoEditTriggers);


    ledsModel = new QStandardItemModel ;
    ledsModel->insertColumns(0,4);
    ledsModel->insertRows(0,8);
    ledsModel->setHorizontalHeaderLabels( (QStringList()
        << "0" << "1" << "2" << "3"
    ));
    ledsModel->setVerticalHeaderLabels( (QStringList()
        << "0" << "1" << "2" << "3"
        << "4" << "5" << "6" << "7"
    ));
    for ( int row = 0 ; row < 8 ; row += 1 )
     for ( int col = 0 ; col < 4 ; col += 1 ) {
        QStandardItem * item = new QStandardItem();
        ledsModel->setItem( row, col, item );
     }
    ui->tvIO->setModel( ledsModel ) ;
    ui->tvScratchpad->setFont(fixedFont);
    for ( int row = 0 ; row < 8 ; row += 1 )
        ui->tvIO->setRowHeight(row, 16 );
    for ( int col = 0 ; col < 4 ; col += 1 )
        ui->tvIO->setColumnWidth(col, 28);
    ui->tvIO->setEditTriggers(QAbstractItemView::NoEditTriggers);


    // LEDs
    for ( int io = 0x00 ; io < 0x04 ; io += 1 ) {
        LEDs * leds = new LEDs() ;
        for ( int bits = 0 ; bits < 8 ; bits += 1 )
            leds->setItem( bits, ledsModel->item( bits, io ) ) ;
        leds->update();
        pBlaze->setIOdevice( this, io, io, leds ) ;
    }


    // UART, lives in 2 places
    UART_IN = new QQueue<uint32_t>() ;
    pBlaze->setIOdevice( this, 0xEC, 0xED, new UART() ) ;


    // CC, lives in 16 places
    CC * cc = new CC() ;
    cc->pBlaze = pBlaze ;
    pBlaze->setIOdevice( this, 0xC0, 0xCF, cc ) ;


    // SBOX
    pBlaze->setIOdevice( this, 0xF0, 0xF0, new SBOX() ) ;


    timer = new QTimer( this ) ;
    connect( timer, SIGNAL(timeout()), this, SLOT(OneStep()));

    connect( fileWatch, SIGNAL(fileChanged(const QString&)), this, SLOT(fileWatch_fileChanged(const QString)) ) ;
}

// destructor
MainWindow::~MainWindow()
{
    pBlaze->resetPB6();
    delete ui;
}

// reload LST+SCR files
void MainWindow::on_actionRefresh_triggered()
{
    if ( fileNames.count() > 0 )
        loadLSTfile( fileNames[ 0 ] ) ;
}

// select LST file
void MainWindow::on_action_Open_triggered()
{
    QFileDialog dialog( this ) ;
    dialog.setDirectory("C:\\Users\\henk\\Documents\\Projects\\PicoChaze6");

    QStringList filters;
    filters << "Picoblaze list files (*.lst)"
            << "Any files (*.*)";
    dialog.setNameFilters(filters);

    if ( dialog.exec() ) {
        fileNames = dialog.selectedFiles();
        loadLSTfile( fileNames[ 0 ] ) ;
    }
}

void MainWindow::fileWatch_fileChanged( const QString &path )
{
    QMessageBox mb ;
    mb.setStandardButtons( QMessageBox::Yes | QMessageBox::No ) ;
    mb.setText( ".LST file: " + path + " changed; reload?" );
    if ( mb.exec() )
        loadLSTfile( path ) ;
}

// load LST+SCR files, build code model/view
void MainWindow::loadLSTfile( QString filename )
{
    QFile file( filename ) ;

    if ( fileWatch->files().count() > 0 )
        fileWatch->removePaths(fileWatch->files());

    // reset core
    on_actionReset_triggered() ;

    ui->actionStep->setEnabled( false ) ;
    ui->actionRun->setEnabled( false ) ;
    ui->actionReset->setEnabled( false ) ;

    // remove code
    codeModel->setRowCount( 0 );
//    ui->tvCode->update() ;
    pBlaze->clearCode();

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    fileWatch->addPath( filename ) ;

    // read .lst file and find code lines
    int row = 0 ;
    while ( !file.atEnd() ) {
         QStandardItem * item ;

         QByteArray line = file.readLine();
         QString str = line ;
         str.remove(QChar('\n'));

         codeModel->insertRow(row);

         item = new QStandardItem(QString("%1").arg(row+1));
         item->setSelectable( true ) ;
         codeModel->setItem( row, 0, item ) ;

         if ( str.contains( QRegExp( "^[0-9A-F]{3,3} [0-9A-F]{5,5}" ) ) ) {
             item = new QStandardItem(*blueIcon, "");
             item->setSelectable( true ) ;

             bool error ;
             int address = str.left(3).toInt(&error, 16) ;
             int code = str.mid(4,5).toInt(&error, 16) ;

             item->setData( QVariant( address ), Qt::UserRole+1 ) ;
             pBlaze->setCodeItem( address, code, row + 1, item ) ;

             codeModel->setItem( row, 1, item ) ;
         } else {
             item = new QStandardItem( "" ) ;
             item->setSelectable( true ) ;
             codeModel->setItem( row, 1, item ) ;
         }

         item = new QStandardItem( str.mid(0, 11) ) ;
         item->setSelectable( true ) ;
         codeModel->setItem( row, 2, item ) ;

         item = new QStandardItem( str.mid(11) ) ;
         item->setSelectable( true ) ;
         codeModel->setItem( row, 3, item );
         row += 1;
     }
    file.close();

    QFileInfo fileInfo( fileNames[ 0 ] ) ;
    QString fp =  fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".scr" ;
    file.setFileName( fp ) ;

    // read scratchpad data (.scr) assciated with .lst
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
         return;
    for ( int addr = -1 ; addr < MAXMEM && !file.atEnd() ; ) {
        bool error ;
        QByteArray line = file.readLine();
        QString str = line ;
        str.remove(QChar('\n'));

        if ( str[ 0 ] == '@' ) {
            addr = str.mid(1).toInt( &error, 16 ) & 0xFF ;
        } else {
            int value = str.toInt(&error, 16) ;
            pBlaze->setScratchpadData( addr, value ) ;
            addr += 1 ;
        }
    }
    file.close();

    ui->actionStep->setEnabled( true ) ;
    ui->actionJump->setEnabled( true ) ;
    ui->actionRun->setEnabled( true ) ;
    ui->actionReset->setEnabled( true ) ;
    on_actionReset_triggered();

    statusBar()->showMessage(tr("Ready"));
}

// exit application
void MainWindow::on_actionExit_triggered()
{
    timer->stop();
    pBlaze->resetPB6();
    qApp->exit();
}

// select/deselect helper
void MainWindow::SelectLine( QItemSelectionModel::SelectionFlags option ) {
    QStandardItem * item = pBlaze->getCurrentCodeItem() ;
    if ( item != NULL ) {
        QItemSelectionModel * selection = ui->tvCode->selectionModel() ;
        selection->select( item->index(), option | QItemSelectionModel::Rows ) ;
        ui->tvCode->scrollTo( item->index(), QAbstractItemView::EnsureVisible);
    }
}

// simulation loop
void MainWindow::OneStep( void ) {
    // no need to report progress everytime
    if ( span < 0 ) {
        pBlaze->updateData() ;
        pBlaze->updateState() ;
        pBlaze->updateIO() ;
        span += 256 ;
    }
    span -= 1 ;

    // run for a short time or until broken
    for ( int i = 0 ; i < 256 ; i += 1 ) {
        if ( ! pBlaze->stepPB6() || pBlaze->onBreakpoint() || pBlaze->onBarrier() ) {
            if ( pBlaze->onBarrier() )
                pBlaze->resetBarrier() ;
            timer->stop();
            pBlaze->updateData() ;
            pBlaze->updateState() ;
            pBlaze->updateIO() ;

            SelectLine( QItemSelectionModel::Select ) ;

            ui->actionStop->setEnabled( false ) ;
            ui->actionJump->setEnabled( true ) ;
            ui->actionStep->setEnabled( true ) ;
            ui->actionRun->setEnabled( true ) ;

            ui->tvCode->setFocus();
            statusBar()->showMessage(tr("Breakpoint") );
            break ;
        }
    }
}

// step into
void MainWindow::on_actionStep_triggered()
{
    SelectLine( QItemSelectionModel::Clear ) ;

    pBlaze->stepPB6() ;
    pBlaze->updateData();
    pBlaze->updateState() ;
    pBlaze->updateIO() ;

    SelectLine( QItemSelectionModel::Select ) ;
    statusBar()->showMessage(tr("Stepped"));
}

// step over
void MainWindow::on_actionJump_triggered()
{
    SelectLine( QItemSelectionModel::Clear ) ;

    ui->actionStep->setEnabled( false ) ;
    ui->actionJump->setEnabled( false ) ;
    ui->actionRun->setEnabled( false ) ;
    ui->actionStop->setEnabled( true ) ;

    pBlaze->setBarrier() ;
    span = 0 ;
    timer->start() ;
    statusBar()->showMessage(tr("Running"));
}

// start simulation
void MainWindow::on_actionRun_triggered()
{
    SelectLine( QItemSelectionModel::Clear ) ;

    ui->actionStep->setEnabled( false ) ;
    ui->actionJump->setEnabled( false ) ;
    ui->actionRun->setEnabled( false ) ;
    ui->actionStop->setEnabled( true ) ;

    span = 0 ;
    timer->start() ;
    statusBar()->showMessage(tr("Running"));
}

// reset simulation
void MainWindow::on_actionReset_triggered()
{
    SelectLine( QItemSelectionModel::Clear ) ;

    pBlaze->resetPB6() ;
    pBlaze->updateData() ;
    pBlaze->updateState() ;
    pBlaze->updateIO() ;

    SelectLine( QItemSelectionModel::Select ) ;

    ui->actionStep->setEnabled( true ) ;
    ui->actionJump->setEnabled( true ) ;
    ui->actionRun->setEnabled( true ) ;
    ui->actionStop->setEnabled( false ) ;
    statusBar()->showMessage(tr("Reset"));
}

// stop simulation
void MainWindow::on_actionStop_triggered() {
    timer->stop() ;
    pBlaze->updateData() ;
    pBlaze->updateState() ;
    pBlaze->updateIO() ;

    SelectLine( QItemSelectionModel::Select ) ;

    ui->actionStep->setEnabled( true ) ;
    ui->actionJump->setEnabled( true ) ;
    ui->actionRun->setEnabled( true ) ;
    ui->actionStop->setEnabled( false ) ;
    statusBar()->showMessage(tr("Stopped"));
}

// modify register
void MainWindow::on_tvRegisters_doubleClicked(const QModelIndex &index) {
    bool ok ;
    // item we clicked on
    QStandardItem * item = registerModel->itemFromIndex( index ) ;
    int row = item->row();

    // get user input
    QString value = QInputDialog::getText(
        this, "Register", "New value of s" + QString("%1").arg(row,2,16).toUpper(),
        QLineEdit::Normal,
        QString("%1").arg( pBlaze->getRegisterValue( row ), 2, 16 ).toUpper(), &ok
    ) ;
    // if valid, put in the register
    if ( ok )
        pBlaze->setRegisterValue( row, value.toInt( &ok, 16 ) ) ;
}

// modify scratchpad cell
void MainWindow::on_tvScratchpad_doubleClicked(const QModelIndex &index) {
    bool ok ;
    QStandardItem * item = scratchpadModel->itemFromIndex( index ) ;
    int row = item->row();
    int col = item->column();

    int cell = row*16+col ;

    QString value = QInputDialog::getText(
        this, "Scratchpad", "New value at 0x" + QString("%1").arg(cell,2,16).toUpper(),
        QLineEdit::Normal,
        QString("%1").arg( pBlaze->getScratchpadValue( cell ), 2, 16 ).toUpper(), &ok
    ) ;
    if ( ok )
        pBlaze->setScratchpadValue( cell, value.toInt( &ok, 16 ) ) ;
}

// toggle LEDs
void MainWindow::on_tvIO_doubleClicked(const QModelIndex &index) {
    QStandardItem * item = ledsModel->itemFromIndex( index ) ;
    int row = item->row() ;
    int col = item->column() ;

    IODevice * dev = pBlaze->getIODevice( col ) ;
    int io = dev->getValue( col ) ;
    dev->setValue( col, io ^ ( 1 << row ) ) ;
    dev->update() ;
}

// toggle breakoint
void MainWindow::on_tvCode_doubleClicked(const QModelIndex &index) {
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

    // set or reset the breakpoint at that address
    if ( pBlaze->getBreakpoint( addr ) ) {
        item->setData( *blueIcon, Qt::DecorationRole ) ;
        pBlaze->resetBreakpoint( addr ) ;
    } else {
        item->setData( *redIcon, Qt::DecorationRole ) ;
        pBlaze->setBreakpoint( addr ) ;
    }
}

// remove all breakpoints
void MainWindow::on_actionRemove_triggered() {
    for ( int addr = 0 ; addr < MAXMEM ; addr += 1 ) {
        if ( pBlaze->getBreakpoint( addr ) ) {
            QStandardItem * item = pBlaze->getCodeItem( addr ) ;
            item->setData( *blueIcon, Qt::DecorationRole ) ;
            pBlaze->resetBreakpoint( addr ) ;
        }
    }
}

// UART to terminal (QPlainTextView in crippled mode) support

uint32_t MainWindow::getUARTstatus( void ) {
    // only the data available bit
    if ( UART_IN->isEmpty() )
        return 0x00 ;
    else
        return 0x20 ;
}

uint32_t MainWindow::getUARTdata( void ) {
    // get the character
    return UART_IN->dequeue() ;
}

void MainWindow::setUARTdata( uint32_t c ) {
    // put it on screen (skip new lines)
    if ( (char)c != '\n' ) {
        ui->teTerminal->insertPlainText( QString( QChar(c) ) ) ;
        ui->teTerminal->ensureCursorVisible();
    }
}

void MainWindow::on_actionAbout_triggered() {
    QMessageBox mb ;
    mb.setStandardButtons( QMessageBox::Ok ) ;
    mb.setText("http://www.mediatronix.com");
    mb.setInformativeText("mailto::info@mediatronix.com");
    mb.exec();
}
