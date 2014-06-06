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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>

#include <QMainWindow>
#include <QStringList>
#include <QStringListModel>
#include <QFileSystemModel>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QFileDialog>
#include <QFont>
#include <QHeaderView>
#include <QMouseEvent>
#include <QItemSelectionModel>
#include <QQueue>
#include <QTimer>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileSystemWatcher>
#include <QSettings>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTreeWidgetItem>
#include <QComboBox>
#include <QCheckBox>
#include <QtScript>
#include <QScriptValue>
#include <QScriptEngineDebugger>


#include "pBlazeQt.h"
#include "hexspinbox.h"
#include "qmtxpicoterm.h"

#include "qmtxledbox.h"
#include "qmtxscriptuart.h"
#include "qmtxscriptcore.h"

#include "qscripthighlighter.h"

#ifdef OSXVERSION
#define PBLAZSIM_VERSION_STR "2.1 beta OSX"
#endif
#ifdef WIN32VERSION
#define PBLAZSIM_VERSION_STR "2.1 beta Win32"
#endif

#define COL_WIDTH 40


namespace Ui {
class MainWindow ;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    friend class IODevice ;

public:
    explicit MainWindow( QWidget *parent = 0 ) ;
    ~MainWindow() ;

    QmtxPicoblaze pBlaze ;
    QmtxPicoTerm picoTerm ;

public slots:
    void updateUI( enum pbState state ) ;

    // PicoTerm
    void showChar( uint32_t c ) ;
    void clearScreen( void ) ;
    void dcsLog( QList<uint32_t> l ) ;
    void getVirtualSwitches( void ) ;


private slots:
    void on_actionNew_triggered();
    void on_action_Open_triggered();
    void on_actionRefresh_triggered();
    void on_actionExit_triggered();

    void on_actionReset_triggered();
    void on_actionStop_triggered();
    void on_actionStep_triggered();
    void on_actionRun_triggered();
    void on_actionJump_triggered();
    void on_actionLeave_triggered();

    void on_actionRemove_triggered();

    void on_tvCode_clicked(const QModelIndex &index);

    void on_tvScratchpad_doubleClicked(const QModelIndex &index);
    void on_tvRegisters_doubleClicked(const QModelIndex &index);
    void on_tvCode_doubleClicked(const QModelIndex &index);
    void on_tvStack_doubleClicked(const QModelIndex &index);
    void on_tvState_doubleClicked(const QModelIndex &index);

    void on_actionLoad_triggered();
    void on_actionSave_Script_triggered();

    void on_actionAbout_triggered();
    void on_actionAbout_Qt_triggered();

    void fileWatch_fileChanged(const QString &path);

    void on_actionEvaluate_triggered();

    void on_twIO_currentChanged(int index);

    void on_teScript_textChanged();

    void signalHandlerException( QScriptValue value ) ;

protected:
    void closeEvent( QCloseEvent * event ) ;
    void timerEvent( QTimerEvent * event ) ;

private:
    Ui::MainWindow * ui ;

    QScriptEngine * engine ;
    QScriptEngineDebugger * debugger ;
    QFile scriptFile ;
    QScriptContext * scriptContext ;
    SharedTools::QScriptHighlighter * lexer ;

    QmtxScriptCore * scriptCore ;
    QmtxScriptUART * uart ;

    int timerId ;
    QElapsedTimer timer ;

    QFileSystemWatcher * fileWatch ;
    QFileSystemModel * filesys_model ;
    QMessageBox mbLSTchanged ;

    QStandardItemModel * codeModel ;
    QStandardItemModel * stateModel ;
    QStandardItemModel * stackModel ;
    QStandardItemModel * registerModel ;
    QStandardItemModel * scratchpadModel ;

    QIcon * blueIcon ;
    QIcon * redIcon ;

    void newCode( void ) ;
    void setCore( QmtxPicoblaze::coreType c ) ;
    void setupIO( void ) ;

    void loadLSTfile( QString filename ) ;
    void selectLine( QItemSelectionModel::SelectionFlags option ) ;

public:
    uint32_t getUARTdata( void ) ;
    uint32_t getUARTstatus( void ) ;
    void setUARTdata( uint32_t c ) ;

    uint32_t getScriptValue( uint32_t address ) ;
    void setScriptValue( uint32_t address, uint32_t value ) ;

    void scriptInterrupt( void ) ;
    void scriptAcknowledge( void ) ;
    void scriptSetIntVect( quint32 addr ) ;
    void scriptSetHWBuild( quint8 value ) ;
} ;

#endif // MAINWINDOW_H
