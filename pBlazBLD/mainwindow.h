#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>


#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexerpsm.h>


namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
public slots:

private slots:
    void on_modificationChanged(bool m) ;

    void on_actionAbout_triggered();
    void on_actionExit_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionNew_triggered();

private:
    Ui::MainWindow * ui ;

    QsciScintilla * textEdit ;
    QsciLexer * lexer ;

    QFile * currentFile ;

    bool saveFile(const QString fileName ) ;
    bool maybeSave() ;
    bool saveas() ;
    bool save() ;
} ;

#endif // MAINWINDOW_H
