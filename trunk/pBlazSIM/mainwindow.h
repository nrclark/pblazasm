/*
 *  Copyright � 2003..2012 : Henk van Kampen <henk@mediatronix.com>
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


#include "pBlaze.h"
#include "ioform.h"

#include "hexspinbox.h"
//#include "../qhexedit2/src/qhexedit.h"


namespace Ui {
class MainWindow ;
}

class KeyPressEater ;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    friend class IODevice ;
    friend class KeyPressEater ;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    Picoblaze * pBlaze ;
    QQueue<uint32_t> * UART_IN ;

public slots:

private slots:
    void on_action_Open_triggered();
    void on_actionExit_triggered();

    void on_actionStep_triggered();
    void on_actionReset_triggered();
    void on_actionRun_triggered();
    void on_actionStop_triggered();
    void on_actionJump_triggered();
    void on_actionRemove_triggered();
    void on_actionRefresh_triggered();
    void on_actionAbout_triggered();

    void on_tvScratchpad_doubleClicked(const QModelIndex &index);
    void on_tvRegisters_doubleClicked(const QModelIndex &index);
    void on_tvCode_doubleClicked(const QModelIndex &index);
    void on_tvIO_doubleClicked(const QModelIndex &index);

    void on_tvCode_clicked(const QModelIndex &index);
    void on_tvStack_doubleClicked(const QModelIndex &index);
    void on_tvState_doubleClicked(const QModelIndex &index);
    void on_actionNew_triggered();
    void on_actionAbout_Qt_triggered();

    void fileWatch_fileChanged(const QString &path);
    void oneStep( void );

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;

    QTimer * timer ;
    int span ;

//    QHexEdit * hexEdit ;

    IOForm * formIO ;

    KeyPressEater * eater ;
    QFileSystemWatcher * fileWatch ;

    QFileSystemModel * filesys_model ;

    QStandardItemModel * codeModel ;
    QStandardItemModel * stateModel ;
    QStandardItemModel * stackModel ;
    QStandardItemModel * registerModel ;
    QStandardItemModel * scratchpadModel ;
    QStandardItemModel * ledsModel ;

    QIcon * greenIcon ;
    QIcon * blueIcon ;
    QIcon * redIcon ;

    void loadLSTfile( QString filename ) ;
    void selectLine(QItemSelectionModel::SelectionFlags option);
    void removeCode( void ) ;

public:
    uint32_t getUARTdata( void ) ;
    uint32_t getUARTstatus( void ) ;
    void setUARTdata( uint32_t c ) ;
} ;

class KeyPressEater : public QObject
 {
     Q_OBJECT

 public:
    MainWindow * w ;

 protected:
     bool eventFilter(QObject *obj, QEvent *event);
 };


#endif // MAINWINDOW_H
