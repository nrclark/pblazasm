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
#include <QMenu>
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>


#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexerpsm.h>

#include "projecthandler.h"

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
    void onMarginClicked(int margin, int line, Qt::KeyboardModifiers state);
    void onCursorpositionchanged(int line, int index);

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

    void openRecentFile() ;
    void highlightLogBox() ;

    void on_actionClose_triggered();

private:
    Ui::MainWindow * ui ;

    QsciScintilla * textEdit ;
    QsciLexer * lexer ;
    QPlainTextEdit * logBox ;
    QSyntaxHighlighter * highlighter ;

    QLabel * lbMode ;
    QLabel * lbModified ;
    QLabel * lbPosition ;

    QFile * currentFile ;
    QmtxProjectHandler * projectHandler ;

    QMenu * popup ;
    QAction * recentFileActs[ MAXRECENTFILES ] ;
    QAction * separatorAct ;

    void loadFile(const QString filename) ;
    void updateRecentFileActions() ;
    QString strippedName(const QString &fullFileName);
    void setCurrentFile(const QString &filename);

    bool saveFile(const QString fileName ) ;
    bool maybeSaveFile() ;
    bool saveas() ;
    bool save() ;

    void readSettings();
    void writeSettings();
} ;

#endif // MAINWINDOW_H
