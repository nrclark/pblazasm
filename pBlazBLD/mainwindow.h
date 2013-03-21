#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QLabel>
#include <QCloseEvent>
#include <QSettings>
#include <QProcess>
#include <QDebug>
#include <QLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>

#include "settingshandler.h"
#include "projecthandler.h"
#include "loghighlighter.h"
#include "psmhighlighter.h"
#include "codeeditor.h"

const int MAXRECENTFILES = 8 ;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow() ;
    
    void closeEvent( QCloseEvent *event ) ;

private slots:
    void onModificationchanged(bool m) ;
    void onTextchanged();
    void onCursorpositionchanged();
    void removeSelectedText() ;

    void deleteRecentProjects() ;
    void deleteRecentFiles() ;

    void on_actionAbout_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionNew_triggered();
    void on_actionAssemble_triggered();
    void on_actionAdd_source_file_triggered();
    void on_actionRemove_source_file_triggered();

    void on_actionNew_Project_triggered();
    void on_actionOpen_Project_triggered();
    void on_actionSave_Project_triggered();

    void openRecentProject();
    void openRecentFile() ;
    void highlightLogBox() ;

    void on_actionClose_triggered();
    void on_actionMerge_triggered();
    void on_actionBitfile_triggered();

    void on_actionAbout_Qt_triggered();

private:
    Ui::MainWindow * ui ;

    QTabWidget * tabWidget ;
    CodeEditor * textEdit ;
    QPlainTextEdit * logBox ;
    QSyntaxHighlighter * logHighlighter ;
    QSyntaxHighlighter * psmHighlighter ;
    QSplitter * hSplitter, * vSplitter ;

    QLabel * lbMode ;
    QLabel * lbModified ;
    QLabel * lbPosition ;

    QFile * currentFile ;
    QmtxSettingsHandler * settingsHandler ;
    QmtxProjectHandler * projectHandler ;

    QMenu * popup ;
    QAction * recentProjectActs[ MAXRECENTFILES ] ;
    QAction * deleteRecentProjectsAct ;
    QAction * recentFileActs[ MAXRECENTFILES ] ;
    QAction * deleteRecentFilesAct ;
    QMenu * recentProjectMenu ;
    QMenu * recentFileMenu ;

    void loadFile(const QString filename) ;
    void loadProject(const QString filename);
    void updateRecentlyUsedActions() ;
    QString strippedName(const QString &fullFileName);
    void setCurrentFile(const QString &filename);

    bool saveFile(const QString fileName ) ;
    bool maybeSaveFile() ;
    bool saveAs() ;
    bool save() ;

    void readSettings();
    void writeSettings();
} ;

#endif // MAINWINDOW_H
